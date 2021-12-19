
#include <iostream>
#include <unistd.h>
#include <memory>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <unordered_map>

// ================================================================================================================ //

class File {
public:
  File( std::string name, std::size_t size )
    : m_name( std::move(name) ), m_size( size ) {}
  ~File() = default;

  const std::string& name() const { return m_name; }
  std::size_t size() const { return m_size; }

  void print( const std::string& indent = "" ) const {
    std::cout<<indent << "File '" << m_name <<"' (" << m_size <<  ")"<<std::endl;
  }
  
private:
  std::string m_name;
  std::size_t m_size;
};


class Node {
public:
  Node( std::string name, std::size_t size ) :
    m_name( std::move(name) ), m_size( size ),
    m_occupiedMemory( 0 ), m_nFiles(0) {}
  Node(const Node& other) :
    m_name(other.m_name), m_size(other.m_size),
    m_occupiedMemory(other.m_occupiedMemory),
    m_nFiles(other.m_nFiles) {}
  Node& operator=(const Node& other) {
    m_name = other.m_name;
    m_size = other.m_size;
    m_occupiedMemory = other.m_occupiedMemory;
    m_nFiles = other.m_nFiles;
    return *this;
  }
  ~Node() = default;

  const std::string& name() const { return m_name; }
  std::size_t size() const { return m_size; }

  std::size_t occupiedMemory() const { return m_occupiedMemory; }
  std::size_t freeMemory() const { return m_size - m_occupiedMemory; }
  std::size_t nFiles() const { return m_nFiles; }
  
  bool canAccept( const File& file ) const { return file.size() <= this->freeMemory(); } 
  bool add( const File& file ) {
    if ( not this->canAccept( file ) ) return false;
    m_occupiedMemory += file.size();
    m_nFiles += 1;
    return true;
  }

  void print( const std::string& indent = "") const {
    std::cout<<indent<< "Node '" << m_name << "' (" << this->freeMemory() << "/" << m_size <<  ") [used: " << this->occupiedMemory() << "]"<<std::endl;
  }    
  
private:
  std::string m_name;
  std::size_t m_size;
  std::size_t m_occupiedMemory;
  std::size_t m_nFiles;
};

// ================================================================================================================ //

int Usage();
template< class T > bool processFile( const std::string&,std::vector<T>& );
void allocateNodes( std::unordered_map< std::size_t, std::size_t  >&,
		    std::vector<File>&,
		    std::vector<Node>& );
bool sortFilesBySize( const File& FileA, const File& FileB );
bool sortNodesByMemory( const Node& NodeA,const Node& NodeB );

// ================================================================================================================ // 

int main( int narg, char* argv[] ) {
  
  std::string inputFilesName = "";
  std::string inputNodesName = "";
  std::string outputName = "";

  // ================================================================================== // 
  
  int c;
  while ( (c = getopt (narg, argv, "f:n:o:h") ) != -1) {
    switch (c) {
    case 'h':
      return Usage();
    case 'f':
      inputFilesName = optarg;
      break;
    case 'n':
      inputNodesName = optarg;
      break;
    case 'o':
      outputName = optarg;
      break;
    case '?':
      if (isprint (optopt))
	fprintf (stderr, "Unknown option `-%c'.\n", optopt);
      else
	fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
      return Usage();
    default:
      return Usage();
    }
  }

  
  if ( inputFilesName.empty() ) {
    std::cout<<"### Input missing: file with file names not specified!"<<std::endl;
    return Usage(); 
  }
  
  if ( inputNodesName.empty() ) {
    std::cout<<"### Input missing: file with nodes not specified!"<<std::endl;
    return Usage();
  }

  // ================================================================================== //

  std::vector< File > listOfFiles;
  std::vector< Node > listOfNodes;
  
  // Read Nodes
  if ( not processFile( inputNodesName,listOfNodes ) )
    return Usage();

  // Read Files 
  if ( not processFile( inputFilesName,listOfFiles ) )
    return Usage();

  // Create output
  std::ofstream output;
  if ( not outputName.empty() ) {
    output.open( outputName.c_str() );
    if ( not output.is_open() ) {
      std::cout<<"ERROR: Cannot open output file: "<<outputName<<std::endl;
      return Usage();
    }
  }
  
  // ================================================================================== //

  // Distributing...
  std::unordered_map< std::size_t, std::size_t > distributionPlan;
  allocateNodes( distributionPlan,listOfFiles,listOfNodes );

  // ================================================================================== //
  
  // Writing the output
  for (const auto& [index_f, index_n] : distributionPlan ) {
    std::string message = listOfFiles[index_f].name() + " ";
    message += index_n < listOfNodes.size() ? listOfNodes[index_n].name() : "NULL";
    if ( not outputName.empty() ) output << message.c_str() << "\n";
    else std::cout << message.c_str() << "\n";
  }

  output.close();  
}

// ================================================================================================================ // 

int Usage() {

  std::cout<<""<< std::endl;
  std::cout<<"USAGE:  ./solution <OPTIONS>"<<std::endl;
  std::cout<<"  OPTIONS:"<<std::endl;
  std::cout<<"        -h               Print usage information"<<std::endl;
  std::cout<<"        -f <filename>    [REQUIRED] Specify input file with list of file names     "<<std::endl;
  std::cout<<"        -n <filename>    [REQUIRED] Specify input file with list of nodes          "<<std::endl;
  std::cout<<"        -o <filename>    [OPTIONAL] Specify output file (default: standard output) "<<std::endl;
  std::cout<<""<<std::endl;

  return 0;
}

template< class T > bool processFile( const std::string& fileName,
				      std::vector<T>& objectCollection ) {

  std::ifstream inputFiles;
  inputFiles.open( fileName.c_str() );
  if ( not inputFiles.is_open() ) {
    std::cout<<"ERROR: Cannot open input file: "<<fileName<<std::endl;
    return false;
  }

  std::string line = "";
  try {
    while ( std::getline(inputFiles, line) ) {
      // Skip the line if it is a comment
      if ( line.find("#",0) == 0 ) continue;
      
      std::istringstream iss( line );
      std::string name = "";
      std::string size = "";
      std::string otherArgument = "";
      iss >> name >> size >> otherArgument;

      // This happens if the line is empty, or if it contains only white spaces
      // Skip the line in these cases
      if ( name.empty() ) continue;

      // Check if there are additional elements
      // This should not happen
      if ( not otherArgument.empty() ) {
	std::cout<<"ERROR: Too many arguments in the line: something is wrong in the input file '" << fileName << "'" << std::endl;
	std::cout<<"ERROR: Faulty line: " << iss.str() << std::endl;
	inputFiles.close();
        return false;
      } 

      // Check the size is a positive integer
      int sizeInt = std::stoi(size);
      if ( sizeInt < 0 ) {
	std::cout<<"ERROR: Size is negative: something is wrong in the input file '" << fileName << "'" << std::endl;
	std::cout<<"ERROR: Faulty line: " << iss.str() << std::endl;
	inputFiles.close();
	return false;
      }

      objectCollection.push_back( T(name, sizeInt) );
    }
  } catch ( ... ) {
    std::cout<<"ERROR: Issues while reading input file: '" << fileName << "'" << std::endl;
    inputFiles.close();
    return false;
  }
  
  inputFiles.close();
  return true;
}

void allocateNodes( std::unordered_map<std::size_t, std::size_t>& distributionPlan,
                    std::vector<File>& listOfFiles,
                    std::vector<Node>& listOfNodes ) {

  // Sort Files in decresing order (size). Big files first.
  std::sort( listOfFiles.begin(),listOfFiles.end(),sortFilesBySize );
  // Sort Nodes according to node memory. Nodes with big occupied memory last.
  // In case of two nodes with same occupied memory, the node with big free memory goes first.
  std::sort( listOfNodes.begin(),listOfNodes.end(),sortNodesByMemory );


  // Running on files
  for ( std::size_t i(0); i<listOfFiles.size(); i++ ) {
    const auto &file = listOfFiles.at(i);
    distributionPlan[ i ] = listOfNodes.size();

    // Running on Nodes to allocate the file
    for ( std::size_t j(0); j<listOfNodes.size(); j++ ) {
      auto& node = listOfNodes.at(j);
      if ( not node.canAccept( file ) ) continue;

      node.add( file );
      distributionPlan[ i ] = j;

      // Place the modified Node into the (new) correct position
      for ( int m(j+1); m<listOfNodes.size(); m++ ) {
	auto &other = listOfNodes.at(m);

	if ( node.occupiedMemory() < other.occupiedMemory() ) break;
	if ( node.occupiedMemory() == other.occupiedMemory() &&
	     node.freeMemory() <= other.freeMemory() ) break;
	std::swap( listOfNodes.at( m-1 ),listOfNodes.at( m ) );
      }

      break;
    }
  }

}

bool sortFilesBySize( const File& FileA, const File& FileB ) {
  return FileA.size() > FileB.size();
}

bool sortNodesByMemory ( const Node& NodeA, const Node& NodeB ) {
  if ( NodeA.occupiedMemory() < NodeB.occupiedMemory() ) return true;
  if ( NodeA.occupiedMemory() > NodeB.occupiedMemory() ) return false;
  if ( NodeA.freeMemory() > NodeB.freeMemory() ) return true;
  return false;
}

