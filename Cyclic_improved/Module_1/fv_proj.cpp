/*===========================================================================
  Extraction of feature vectors (EC descriptors)
  Author : Discrete Mathematics Lab, Kyoto University
  Version: 1.0 (November 2020)

  This file implements functions that,
  given a CSV file and an SDF file,
    - calculates the feature vectors for each chemical compound
      in the SDF file,
      where descriptors are defined in the first line
      of the CSV file.

  <CAUTION on CSV file>
    - The first line of the CSV file should contain
      descriptor names in the comma-separated format.
    - The subsequent lines are ignored. 
    - The CSV file is expected to be generated by gen_descriptor_EC.
      A user is strongly recommended to use it. 

  LICENSE: MIT license, see License.txt
  ===========================================================================*/

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <stack>
#include <queue>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <algorithm>

bool debug = false; // Used for debugging during development


/********** MACROS **********/
// if 1, only rho-lean graph is considered
#define LIMIT_TO_RHOLEAN 1

// The following values should NOT be change
#define RHO 2          // Branch parameter rho 

#define NONCORE  1     // Value indicating noncore-vertex/edge
#define CORE     0     // Value indicating core-vertex/edge
#define INTERNAL 1     // Value indicating internal vertex/edge
#define EXTERNAL 2     // Value indicating external vertex/edge
/****************************/

#include "common.cpp"

/***** number of char c in string s *****/
int getNumChar(char s[], char c){
  int numChar = 0;
  int k = 0;
  while( s[k] != '\0' ){
    if( s[k] == c )
      numChar++;
    k++;
  }
  return numChar;
}


/***** functions for memory allocation *****/
void *mallocE(size_t size){
  void *s;
  if ( (s=malloc(size)) == NULL ) {
    fprintf( stderr, "malloc: not enough memory.\n" );
    exit( EXIT_FAILURE );
  }
  return s;
}

/***** split a string into strings *****/
char **split(char s[], char c){
  typedef struct list{
    int i;
    struct list *p;
  } List;
  char **str;
  List head,*tail,*ip;
  int countnum=0,counts=0;
  int i=0,j=0,i0=0;
  head.i=-1;
  head.p=NULL;
  tail=&head;
  while(s[i]!='\0'){
    if(s[i]==c){
      ip=(List*)mallocE(sizeof(List));
      ip->i=countnum;
      ip->p=NULL;
      tail->p=ip;
      tail=ip;
      countnum=-1;
      counts++;
    }
    countnum++;
    i++;
  }
 ip=(List*)mallocE(sizeof(List));
  ip->i=countnum;
  ip->p=NULL;
  tail->p=ip;
  tail=ip;
  str=(char**)mallocE((counts+1)*sizeof(char*));
  ip=&head;
  do{
    int k=0;
    ip=ip->p;
    str[j]=(char*)mallocE(((ip->i)+1)*sizeof(char));
    for(i=i0;i<i0+ip->i;i++){
      str[j][k]=s[i];
      k++;
    }
    str[j][k]='\0';
    i0+=ip->i+1;
    j++;
  }while(ip->p!=NULL);
  // free memory
  tail = head.p;
  while( tail != NULL ){
    ip = tail->p;
    free( tail );
    tail = ip;
  }
  return str;

}




/***** Core function of this program: Other parts are almost the same as gen_descriptor_EC *****/
void printFeatureVec_Desc(char **names_descs,
			  int &num_descs,
			  size_t &num_instance,
			  vector < string > &f_name,
			  vector < double > f_value[],
			  string desc_filename,
			  string input_filename,
			  string out_file_name
			  ){

  // map from names_desc to f_name
  int *map_desc = new int[num_descs];
  int exists = 0;
  for(int k=0;k<num_descs;k++){
    map_desc[k] = -1; // -1 means names_descs[k] does not appear in f_name. in this case, feature value is set to zero.
    for(size_t j=0;j<f_name.size();j++)
      if(names_descs[k] == f_name[j]){
	map_desc[k] = j;
	exists++;
	break;
      }
  }

  // output
  cout << desc_filename << ": " << num_descs << " features\n";
  cout << input_filename << ": " << f_name.size() << " features\n";
  cout << "# of common features: " << exists << "\n";
  cout << "Non-existing " << num_descs-exists << " features:";
  for(int k=0;k<num_descs;k++)
    if(map_desc[k]==-1)
      cout << " " << names_descs[k];
  cout << "\nFor these features, values are set to 0.\n\n";
  
  // debug: output correspondence table
  if(debug){
    cout << "------------------------------\n";
    cout << desc_filename << "\t" << input_filename << "\n";
    for(int k=0;k<num_descs;k++){
      cout << k << "\t" << names_descs[k] << "\t\t";
      if(map_desc[k]!=-1)
	cout << map_desc[k] << "\t" << f_name[map_desc[k]] << "\n";
      else
	cout << "-1\n";
    }
    cout << "------------------------------\n\n";
  }
  
  // write out feature vector to the supplied file
  ofstream outputfile(out_file_name, ios::out);

  for (size_t k = 0; k < num_descs-1; ++k) {
    outputfile << names_descs[k] << ",";
  }
  outputfile << names_descs[num_descs-1] << "\n";

  for (size_t i = 0; i < num_instance; ++i) {
    for (size_t k = 0; k < num_descs; ++k) {
      string endstr;
      if(k==num_descs-1)
	endstr = "\n";
      else
	endstr = ",";
      if(map_desc[k]!=-1){
	size_t j = map_desc[k];
	if (j == 0) outputfile << (int) f_value[i][j] << endstr;
	else outputfile << f_value[i][j] << endstr;
      }
      else
	outputfile << "0" << endstr;
    }
  }

  outputfile.close();

  delete[] map_desc;
}



int main(int argc, char **argv){

  /*** process input arguments ***/
  if (argc != 4) {
    fprintf(stderr, "usage: %s (DESCRIPTOR.csv)(INPUT.sdf)(OUTPUT.csv)\n", argv[0]);
    return(1);
  }
  // Check if RHO=2
  if (RHO != 2) {
    fprintf(stderr, "error: in this version, RHO should be 2.\n");
    exit(1);
  }
  string desc_filename = argv[1];
  string input_filename = argv[2];
  string output_filename = argv[3];

  /*** read descriptor names ***/
  char **names_descs,str[4096];
  FILE *fp;
  int num_descs;
  fp = fopen(argv[1], "r");
  fscanf(fp, "%s", str);
  names_descs = split(str, ',');
  num_descs = getNumChar(str,',')+1;
  fclose(fp);
  
  /*** initialize ***/
  // a map with atomic mass for different elements
  MassMap massMap = init_MassMap();
  // a map for storing valence of different atoms
  ValenceMap valMap = init_ValenceMap();

  /*** read chemical graphs from the input sdf ***/
  vector < Graph > H;
  try {
    H = read_graph_sdf(input_filename);
  } catch (const exception &e) {
    cout << "Error in reading input file.\n" <<
      e.what() << endl <<
      "The program will now terminate." << endl;
    return(1);
  }


  // Prepare storage for H-suppressed graphs
  vector < Graph > G;
  vector < size_t > num_H_atoms; 	 // Number of H-atoms
  size_t num_graphs;
  size_t num_H = H.size();

  
  // check whether each graph is RHO-lean, converting it into H-suppressed model
  for (size_t i = 0; i < num_H; ++i) {
    Graph g;
    size_t h;
    h = H_suppressed_convert(H[i], g);
    if(LIMIT_TO_RHOLEAN == 0 || is_feasible_graph(g) == true){
      G.push_back(g);
      num_H_atoms.push_back(h);
    }
    else{
      cerr << "Warning: CID " << g.CID << " is not feasible; i.e., non-" << RHO << "-lean or acyclic.\n";
    }
  }  
  num_graphs = G.size();

  // Prepare storage for each descriptor
  vector < size_t > diameter(num_graphs);	 // Graph diameter
  vector < size_t > sumDist(num_graphs);	 // Stores sum of distances
  vector < size_t > num_atom(num_graphs); 	 // Number of non-hydrogen atoms
  vector < size_t > double_bond_in(num_graphs);	 // Number of internal double bonds
  vector < size_t > double_bond_ex(num_graphs);	 // Number of external double bonds
  vector < size_t > triple_bond_in(num_graphs);  // Number of internal triple bonds
  vector < size_t > triple_bond_ex(num_graphs);  // Number of external triple bonds
  vector < double > M(num_graphs);	 	 // Sum of atomic mass
  vector < size_t > branch_height(num_graphs); // Branch height
  vector < size_t > branch_leaf_number(num_graphs);   // Branch-leaf number
  // Introduce integer IDs in mappings
  vector < map < string, size_t > > element_map_in(num_graphs);
  vector < map < string, size_t > > element_map_ex(num_graphs);
  vector < map < string, size_t > > onepath_map_in(num_graphs);
  vector < map < string, size_t > > onepath_map_ex(num_graphs);
  vector < map < string, size_t > > mult_path_map(num_graphs);

  vector < string > Lambda;
  Lambda.push_back("C");	// Assign carbon to the first order
  vector < string > globalOnePath;
  vector < string > globalMoreThanOnePath;
  vector < vector < size_t > > degree_vertices_in(num_graphs);
  vector < vector < size_t > > degree_vertices_ex(num_graphs);

  vector < size_t > core_size(num_graphs);
  vector < size_t > core_height(num_graphs);
  vector < vector < size_t > > degree_vertices_CO(num_graphs);
  vector < vector < size_t > > degree_vertices_NC(num_graphs);
  vector < vector < vector < size_t > > > BDM(num_graphs); // number of core/internal/external-edges with beta=2 and 3
  vector < vector < unordered_map < string, size_t > > > NS(num_graphs); // number of core/noncore-vertices with prescribed degree
  vector < vector < string > > Lambda_C(2); // \Lambda^{co,nc}_dg
  vector < vector < unordered_map < string, size_t > > > EC(num_graphs); // edge configuration
  vector < vector < string > > Gamma(3); // \Gamma^{co,in,ex}
  
  Lambda_C[CORE].clear();
  Lambda_C[NONCORE].clear();
  Gamma[CORE].clear();
  Gamma[INTERNAL].clear();
  Gamma[EXTERNAL].clear();
  
  
  for (size_t i = 0; i < num_graphs; ++i) {

    Graph _g = G[i];
    cutTree(_g, RHO);

    Graph __g = G[i];
    getCoreInfo(__g, core_size[i], core_height[i], degree_vertices_CO[i], degree_vertices_NC[i], BDM[i]);
    
    try {
      calcDegree(G[i],
		 degree_vertices_in[i],
		 degree_vertices_ex[i],
		 _g.status );
      
      calcDegreeNS(G[i], NS[i], Lambda_C, __g.branch_status);
    
    } catch (const exception& e) {
      cout << "Exception caught in calculating degree:" << endl;
      cout << e.what() << endl;
      cout << "The program will now exit" << endl;
      return (1);
    }
    num_atom[i] = G[i].numAtom;
    calcDoubleTripleBond(G[i], double_bond_in[i], double_bond_ex[i],
			 triple_bond_in[i], triple_bond_ex[i], _g.status);
    try {
      M[i] = calcM(G[i], massMap);
    } catch (const exception &ex) {
      cerr << "An exception when calculating the mass "
	   << "of graph with id " << G[i].CID << endl;
      cerr << "Perhaps it contains an element not registered "
	   << "in the table of atomic masses?" << endl;
      cerr << ex.what() << endl;
      cerr << "The program will now terminate" << endl;
      return 1;
    }
    getElementCount(G[i],
		    element_map_in[i],
		    element_map_ex[i],
		    Lambda,
		    _g.status);
    calcBranchHeight(G[i], 2, branch_height[i], branch_leaf_number[i]);
    calcEdgeConfiguration(G[i], massMap, EC[i], Gamma, __g.branch_status, __g.ht);
  }

  map<string, size_t> elementOrder;
  assignElemOrderByMass(Lambda, massMap, elementOrder);
  
  vector <map <vector <size_t>, size_t>> bond_conf_in;
  vector <map <vector <size_t>, size_t>> bond_conf_ex;

  for (size_t i = 0; i < num_graphs; ++i) {
    Graph _g = G[i];
    cutTree(_g, 2);
    getOnePath( G[i], elementOrder, onepath_map_in[i], onepath_map_ex[i],
		globalOnePath, _g.status );
    int sumD, dia;
    calcDist(G[i], sumD, dia);
    sumDist[i] = sumD;
    diameter[i] = dia;
    map <vector <size_t>, size_t> bond_conf_t_in;
    bond_conf_in.push_back(bond_conf_t_in);
    map <vector <size_t>, size_t> bond_conf_t_ex;
    bond_conf_ex.push_back(bond_conf_t_ex);
    calcBondConfiguration(G[i], bond_conf_in[i],
			  bond_conf_ex[i], _g.status);
  }

  for(size_t i=0; i<num_graphs; i++)
    if(num_H_atoms[i] == 0){
      size_t valence_sum = 0;
      size_t bond_sum = 0;
      for(Vertex u=0; u<G[i].numAtom; u++)
	valence_sum += valMap.at(G[i].alpha[u]);
      for(Vertex u=0; u<G[i].numAtom; u++)
	for(Vertex v=0; v<G[i].numAtom;v++)
	  bond_sum += G[i].beta[u][v];
      num_H_atoms[i] = valence_sum - bond_sum;
      if(debug)
	cout << valence_sum << " " << bond_sum << "\n";
    }
 

  // A vector that stores the descriptor names
  vector < string > f_name;
  // A 2D structure to store the descriptor values for each input graph
  vector < double > f_value[num_graphs];

  // Store the feature vector values in a 2D structure
  getFeatureVecCyclic(G, f_name, f_value,
		      num_atom, core_size, core_height, branch_leaf_number,
		      M, degree_vertices_CO, degree_vertices_NC, BDM,
		      NS, Lambda_C, EC, Gamma, num_H_atoms);
  
  // Print the stored values to a given file
  //printFeatureVec(num_graphs, f_name, f_value, output_filename);
  printFeatureVec_Desc(names_descs, num_descs, num_graphs, f_name, f_value, desc_filename, input_filename, output_filename);

  // Report that the program has finished and exit
  cout << "For " << num_H << " compounds in " << input_filename << ",\n";
  cout << num_graphs << " feature vectors have been written to " <<
    output_filename << endl;
  
  return 0;
}
