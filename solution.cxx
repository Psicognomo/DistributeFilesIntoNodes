
#include "iostream"
#include "unistd.h"
#include "memory"
#include "fstream"
#include "sstream"
#include "sstream"
#include "vector"
#include "map"

// ================================================================================================================ //

class File {
public:
  File( const std::string& name, int size ) :
    m_name( name ), m_size( size ) {}
  ~File() {}

  const std::string name() const { return m_name; }
  int size() const { return m_size; }

  void print( const std::string& indent = "" ) const {
    std::cout<<indent << "File '" << m_name <<"' (" << m_size <<  ")"<<std::endl;
  }
  
private:
  const std::string m_name;
  int m_size;
};


class Node {
public:
  Node( const std::string& name, int size ) :
    m_name( name ), m_size( size ),
    m_occupiedMemory( 0 ) {}
  ~Node() {}

  const std::string name() const { return m_name; }
  int size() const { return m_size; }

  int occupiedMemory() const { return m_occupiedMemory; }
  int freeMemory() const { return m_size - m_occupiedMemory; }
  int nFiles() const { return m_files.size(); }
  
  bool canAccept( const File* file ) const { return file->size() <= this->freeMemory(); } 
  bool add( const File* file ) {
    if ( this->canAccept( file ) == false ) return false;
    m_occupiedMemory += file->size();
    m_files.push_back( file );
    return true;
  }

  void print( const std::string& indent = "") const {
    std::cout<<indent<< "Node '" << m_name << "' (" << this->freeMemory() << "/" << m_size <<  ") [used: " << this->occupiedMemory() << "]"<<std::endl;
    std::cout<<indent<< "  Stored Files: " << m_files.size()<<std::endl;
    for ( int i(0); i<m_files.size(); i++ ) 
      m_files.at(i)->print( "    * " );
  }    
  
private:
  const std::string m_name;
  int m_size;
  int m_occupiedMemory;
  std::vector< const File* > m_files; 
};

// ================================================================================================================ //

int Usage();
void allocateNodes( std::map< std::string,std::string  >&,
		    std::vector< std::shared_ptr< File > >&,
		    std::vector< std::shared_ptr< Node > >& );
bool sortFilesBySize( std::shared_ptr< File >& FileA,std::shared_ptr< File >& FileB );
bool sortNodesByAvailableMemory( std::shared_ptr< Node >& NodeA,std::shared_ptr< Node >& NodeB );

// ================================================================================================================ // 

int main( int narg, char* argv[] ) {

  std::string inputFilesName = "";
  std::string inputNodesName = "";
  std::string outputName = "result.txt";

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

  // ================================================================================== // 
  
  if ( inputFilesName.empty() ) {
    std::cout<<"### Input missing: file with file names not specified!"<<std::endl;
    return Usage(); 
  }
  
  if ( inputNodesName.empty() ) {
    std::cout<<"### Input missing: file with nodes not specified!"<<std::endl;
    return Usage();
  }

  std::cout<<std::endl << "I/O SUMMARY:"<<std::endl;
  std::cout<<"   INPUT : file with file names: " << inputFilesName << std::endl;
  std::cout<<"   INPUT : file with nodes     : " << inputNodesName << std::endl;
  std::cout<<"   OUTPUT: output file         : " << outputName     << std::endl << std::endl;

  std::ifstream inputFiles;
  inputFiles.open( inputFilesName.c_str() );
  if ( not inputFiles.is_open() ) {
    std::cout<<"ERROR: Cannot open input file: "<<inputFilesName<<std::endl;
    return Usage();
  }

  std::ifstream inputNodes;
  inputNodes.open( inputNodesName.c_str() );
  if ( not inputNodes.is_open() ) {
    std::cout<<"ERROR: Cannot open input file: "<<inputNodesName<<std::endl;
    return Usage();
  }

  std::ofstream output;
  output.open( outputName.c_str() );
  if ( not output.is_open() ) {
    std::cout<<"ERROR: Cannot open output file: "<<outputName<<std::endl;
    return Usage();
  }
  
  // ================================================================================== //

  std::vector< std::shared_ptr< File > > listOfFiles;
  std::vector< std::shared_ptr< Node > >  listOfNodes;

  // Read Nodes
  std::string line;
  try {
    while ( std::getline(inputNodes, line) ) {
      if ( line.find("#",0) == 0 ) continue; 
      
      std::istringstream iss( line );
      std::string name, size;
      iss >> name >> size;
      
      std::shared_ptr< Node > toAdd( new Node( name,std::stoi(size) ) );
      listOfNodes.push_back( toAdd );
    }
  } catch ( ... ) {
    std::cout<<"ERROR: Issues while reading input file: " << inputNodesName << std::endl;
    return Usage();
  }
  
  // Read Files
  try {
    while( std::getline(inputFiles, line) ) {
      if ( line.find("#",0) == 0 ) continue;
      
      std::istringstream iss( line );
      std::string name, size;
      iss >> name >> size;

      std::shared_ptr< File > toAdd( new File( name,std::stoi(size) ) );
      listOfFiles.push_back( toAdd );
    }
  } catch ( ... ) {
    std::cout<<"ERROR: Issues while reading input file: " << inputFilesName << std::endl;
    return Usage();
  }
  
  std::cout<<"Found a total of " << listOfNodes.size() << " Nodes "<<std::endl;
  std::cout<<"Found a total of " << listOfFiles.size() << " Files "<<std::endl;
  std::cout<<"Distributing..."<<std::endl;
  
  std::map< std::string,std::string > distributionPlan;
  allocateNodes( distributionPlan,listOfFiles,listOfNodes );

  // ================================================================================== //
  
  std::cout<<"Writing into output file"<<std::endl<<std::endl;

  std::map< std::string,std::string >::const_iterator it = distributionPlan.begin();
  for ( ; it != distributionPlan.end(); it++ ) {
    std::string message = it->first + " " + it->second;
    output << message.c_str() << "\n";
  }

  // ================================================================================== // 

  inputFiles.close();
  inputNodes.close();
  output.close();  
}

// ================================================================================================================ // 

int Usage() {

  std::cout<<""<< std::endl;
  std::cout<<"USAGE:  ./solution <OPTIONS>"<<std::endl;
  std::cout<<"  OPTIONS:"<<std::endl;
  std::cout<<"        -h               Print usage information"<<std::endl;
  std::cout<<"        -f <filename>    [REQUIRED] Specify input file with list of file names"<<std::endl;
  std::cout<<"        -n <filename>    [REQUIRED] Specify input file with list of nodes     "<<std::endl;
  std::cout<<"        -o <filename>    [OPTIONAL] Specify output file (default: result.txt) "<<std::endl;
  std::cout<<""<<std::endl;

  return 0;
}

void allocateNodes( std::map< std::string,std::string  >& distributionPlan,
                    std::vector< std::shared_ptr< File > >& listOfFiles,
                    std::vector< std::shared_ptr< Node > >& listOfNodes ) {

  // Sort Files in decresing order (size)
  std::sort( listOfFiles.begin(),listOfFiles.end(),sortFilesBySize );
  // Sort Nodes according to available memory (increasing order)
  std::sort( listOfNodes.begin(),listOfNodes.end(),sortNodesByAvailableMemory );

  
  for ( int i(0); i<listOfFiles.size(); i++ ) {
    std::shared_ptr< File >& file = listOfFiles.at(i);
    distributionPlan[ file.get()->name() ] = "NULL";
    
    int selectedNodePosition = -1;    
    int newNodePosition = listOfNodes.size() - 1;
  
    for ( int j(0); j<listOfNodes.size(); j++ ) {
      std::shared_ptr< Node >& node = listOfNodes.at(j);
      if ( node->canAccept( file.get() ) == false ) continue;

      node->add( file.get() );
      distributionPlan[ file.get()->name() ] = node.get()->name(); 
      selectedNodePosition = j;

      // Find new position
      for ( int m(j+1); m<listOfNodes.size(); m++ ) {
	std::shared_ptr< Node >& other = listOfNodes.at(m) ;

	if ( node->occupiedMemory() > other->occupiedMemory() ) continue;
	if ( node->occupiedMemory() == other->occupiedMemory() ) 
	  if ( node->freeMemory() > other->freeMemory() ) continue;
	newNodePosition = m;
	break;
      }

      break;
    }

    if ( selectedNodePosition == -1 ) continue;
    if ( selectedNodePosition == newNodePosition ) continue;

    std::shared_ptr< Node > toAdd = listOfNodes.at( selectedNodePosition );
    listOfNodes.insert( listOfNodes.begin() + newNodePosition + 1 , toAdd );
    listOfNodes.erase( listOfNodes.begin() + selectedNodePosition );    
  }

}

bool sortFilesBySize( std::shared_ptr< File >& FileA,std::shared_ptr< File >& FileB ) {
  if ( FileA->size() > FileB->size() ) return true;
  return false;
}

bool sortNodesByAvailableMemory (std::shared_ptr< Node >& NodeA,std::shared_ptr< Node >& NodeB ) {
  if ( NodeA->occupiedMemory() < NodeB->occupiedMemory() ) return true;
  else if ( NodeA->occupiedMemory() > NodeB->occupiedMemory() ) return false;
  else {
    if ( NodeA->freeMemory() > NodeB->freeMemory() ) return true;
    return false;
  }
}

