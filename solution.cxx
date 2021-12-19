
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
  File() = delete;
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
  Node() = delete;
  Node(std::string name, std::size_t size) :
    m_name( std::move(name) ), m_size( size ),
    m_occupiedMemory( 0 ) {}
  ~Node() = default;

  const std::string& name() const { return m_name; }
  std::size_t size() const { return m_size; }

  std::size_t occupiedMemory() const { return m_occupiedMemory; }
  std::size_t freeMemory() const { return m_size - m_occupiedMemory; }
  
  bool canAccept( const File& file ) const { return file.size() <= this->freeMemory(); } 
  bool add( const File& file ) {
    if ( not this->canAccept( file ) ) return false;
    m_occupiedMemory += file.size();
    return true;
  }

  void print( const std::string& indent = "") const {
    std::cout<<indent<< "Node '" << m_name << "' (" << this->freeMemory() << "/" << m_size <<  ") [used: " << this->occupiedMemory() << "]"<<std::endl;
  }    
  
private:
  std::string m_name;
  std::size_t m_size;
  std::size_t m_occupiedMemory;
};

// ================================================================================================================ //

int Usage();
template< class T > bool processFile( const std::string&,std::vector<T>& );
void allocateNodes( std::unordered_map< std::string, std::string  >&,
		    std::vector<File>&,
		    std::vector<Node>& );

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
  std::unordered_map< std::string, std::string > distributionPlan;
  allocateNodes( distributionPlan, listOfFiles, listOfNodes );
  
  // ================================================================================== //
    
  // Writing the output
  for( const auto& [file, node] : distributionPlan) {
    std::string message = file + " " + node;
    
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

void allocateNodes( std::unordered_map<std::string, std::string>& distributionPlan,
                    std::vector<File>& listOfFiles,
                    std::vector<Node>& listOfNodes ) {

  distributionPlan.reserve(listOfFiles.size());
  
  std::vector<std::size_t> indexes_files;
  std::vector<std::size_t> indexes_nodes;

  indexes_files.reserve(listOfFiles.size());
  indexes_nodes.reserve(listOfNodes.size());
  
  for (std::size_t i(0); i < listOfFiles.size(); i++)
    indexes_files.push_back(i);
  for (std::size_t i(0); i < listOfNodes.size(); i++)
    indexes_nodes.push_back(i);

  // Compare functions
  auto compareFiles = [&listOfFiles] (std::size_t elA, std::size_t elB) -> bool
		      {
			return listOfFiles.at(elA).size() > listOfFiles.at(elB).size();
		      };
  
  auto compareNodes = [&listOfNodes] (std::size_t elA, std::size_t elB) -> bool
		      {
			const auto& NodeA = listOfNodes.at(elA);
			const auto& NodeB = listOfNodes.at(elB);
			if ( NodeA.occupiedMemory() < NodeB.occupiedMemory() ) return true;
			if ( NodeA.occupiedMemory() > NodeB.occupiedMemory() ) return false;
			if ( NodeA.freeMemory() > NodeB.freeMemory() ) return true;
			return false;
		      };
  
  // Sort Files in decresing order (size). Big files first.
  std::sort( indexes_files.begin(), indexes_files.end(),
	     compareFiles );
  
  // Sort Nodes according to node memory. Nodes with big occupied memory last.
  // In case of two nodes with same occupied memory, the node with big free memory goes first.
  std::sort( indexes_nodes.begin(), indexes_nodes.end(),
	     compareNodes );
  
  // Running on files
  for ( std::size_t idx_f : indexes_files ) {
    const auto &file = listOfFiles.at(idx_f);
    distributionPlan[ file.name() ] = "NULL";

    // Running on Nodes to allocate the file
    for ( std::size_t j(0); j<indexes_nodes.size(); j++ ) {
      std::size_t idex_n_current = indexes_nodes.at(j);

      if ( not listOfNodes.at(idex_n_current).canAccept( file ) ) continue;
      
      listOfNodes.at(idex_n_current).add( file );
      distributionPlan[ file.name() ] = listOfNodes.at(idex_n_current).name();
      
      // Place the modified Node into the (new) correct position
      std::size_t current_pos = j;
      std::size_t target_pos = j;
      
      for (std::size_t m(j+1); m<indexes_nodes.size(); m++, target_pos++) {
    	if ( compareNodes( indexes_nodes.at(current_pos), indexes_nodes.at(m) ) )
    	  break;
      }
      
      std::size_t current_value = indexes_nodes.at(current_pos);
      for ( ; current_pos < target_pos; current_pos++) 
    	indexes_nodes.at(current_pos) = indexes_nodes.at(current_pos + 1);
      indexes_nodes.at(target_pos) = current_value;
          
      break;
    }
  }
}


