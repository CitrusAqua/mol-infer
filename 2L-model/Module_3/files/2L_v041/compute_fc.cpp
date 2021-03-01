#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>

#include "./fringe/TopologyGraph.hpp"
#include "./fringe/RootedGraph.hpp"

/***** the structure TS_with_freq is introduced to sort TS by freq *****/
class TS_with_freq{
public:
  TreeSignature TS;
  int freq;
  TS_with_freq(TreeSignature _TS, int _freq){
    this->TS = _TS;
    this->freq = _freq;
  };
};
  
bool operator<(const TS_with_freq& T1, const TS_with_freq& T2){
  return T1.freq < T2.freq;
};
/***********************************************************************/


void write_result_csv(
		      const string outputfilename, 
		      const unordered_map <TreeSignature, int>& TS_map, 
		      const int& total_fringe_tree_num
		      ){
    
  ofstream outputfile(outputfilename, ios::out);

  outputfile << "# non-isomorphic fringe trees = ," << TS_map.size() << "\n";
  outputfile << "# of all Fringe trees = ," << total_fringe_tree_num << "\n";
  outputfile << "\n";
  outputfile << "(atom depth)-seq, Multiplicity seq, Frequency, Percentage" << "\n";

  for (auto& tmp : TS_map){
    auto& TS = tmp.first;
    auto& freq = tmp.second;

    string str_tmp = "";
    int i = 0;
    for (auto& a : TS.delta){
      if (i % 2 == 0){
	str_tmp += map_int2atomic.at(a) + " ";
      } else {
	str_tmp += to_string(a) + " ";
      }
      ++i;   
    }
    outputfile << str_tmp << ",";
        
    str_tmp = "";
    for (auto& a : TS.mu){
      str_tmp += to_string(a) + " ";
    }
    outputfile << str_tmp << ",";
     
    outputfile << freq << "," << (double)freq / total_fringe_tree_num <<  "\n";      
  }

  outputfile.close();
}

void write_result(
		  const string outputfilename, 
		  const unordered_map <TreeSignature, int>& TS_map, 
		  const int& total_fringe_tree_num
		  ){
    
  ofstream outputfile(outputfilename, ios::out);

  outputfile << TS_map.size() << "\n";
  outputfile << total_fringe_tree_num << "\n";

  for (auto& tmp : TS_map){
    auto& TS = tmp.first;
    auto& freq = tmp.second;

    int i = 0;
    for (auto& a : TS.delta){
      if (i % 2 == 0){
	outputfile << map_int2atomic.at(a) << " ";
      } else {
	outputfile << a << " ";
      }
      ++i;   
    }
    outputfile << "\n";
    for (auto& a : TS.mu){
      outputfile << a << " ";
    }
    outputfile << "\n";
     
    outputfile << freq << " " << (double)freq / total_fringe_tree_num <<  "\n";      
  }

  outputfile.close();
}

void read_data(const string outputfilename, unordered_map <TreeSignature, int>& TS_map, int& total_fringe_tree_num){
  ifstream infile;
  string line;

  try {
    infile.open(outputfilename);
  } catch (const exception& e){
    return;
  }

  if (!infile.is_open()){
    return;
  }

  if (infile.eof()){
    return;
  }

  stringstream st1;
  getline(infile, line);
  st1 << line;
  int n;
  st1 >> n;
  st1.clear();
  getline(infile, line);
  st1 << line;
  st1 >> total_fringe_tree_num;
  st1.clear();

  for (int i = 0; i < n; ++i){
    TreeSignature TS;

    TS.delta.clear();
    TS.mu.clear();

    stringstream st2;
    getline(infile, line);
    st2 << line;

    string str_tmp;
    int tmp2;
    while (st2 >> str_tmp >> tmp2){
      TS.delta.push_back(map_atomic_number.at(str_tmp));
      TS.delta.push_back(tmp2);
    }

    stringstream st3;
    getline(infile, line);
    st3 << line;

    int tmp3;
    while (st3 >> tmp3){
      TS.mu.push_back(tmp3);
    }

    stringstream st4;
    getline(infile, line);
    int freq;
    st4 << line;
    st4 >> freq;

    TS_map.emplace(TS, freq);
  }
    
  infile.close();
}


/********** main of computing fc **********/
void fc_main_v02(string sdf_file,
		 string fringe_file, int rho,
		 vector < string >& fc_CID,
		 vector < string >& fc_ftr_name,
		 vector < vector < double > >& fc_ftr_val){
  int maxd_check = 1;
  ifstream infile;

  // read the sdf file
  try {
    infile.open(sdf_file);
  } catch (const exception& e){
    cerr << "Cannot open '" << sdf_file << "' for reading!" << endl;
    throw(e);
  }
  if (!infile.is_open()){
    cerr << "Error, infile not initialized!" << endl;
    throw(-1);
  }

  // preparation
  unordered_map <string, int> atom_map = map_atomic_number;
  unordered_map <TreeSignature, int> TS_map;
  TS_map.clear();
  map <string, unordered_map <TreeSignature, int>> CID_TS_map;
  CID_TS_map.clear();
  
  int total_fringe_tree_num = 0;

  // read chemical compounds one by one
  while (! infile.eof()){
    ChemicalGraph _g;
    int cond = _g.readFromFile(infile);
    if (cond == 0){
      break;
    }
    if (cond == -1){
      continue;
    }
    _g.HSuppress();
    int n = _g.EffectiveAtomNum();
    if (n == 0){
      continue;
    }
    if (maxd_check != 0 && _g.maxd() > 4){
      continue;
    }
    auto g = RemoveNullVertex(_g);
        
    vector <RootedTree> FTs = getFringeTrees(g, rho);

    unordered_map <TreeSignature, int> TS_map_tmp;
    TS_map_tmp.clear();

    for (auto& RT : FTs){
      ++total_fringe_tree_num;
      auto RT_tmp = RemoveNullVertex(RT);
      RootedGraph RG(RT_tmp);
      RootedMultiTree RMT(RG, RT_tmp);

      for (Vertex u = 0; u < RMT.n; ++u){
	RMT.label[u] = atom_map.at(RT_tmp.graph.alpha[u]);
      }
      TreeSignature TS = RMT.getSignature();
      ++TS_map[TS];
      ++TS_map_tmp[TS];
    }
    CID_TS_map[_g.CID] = TS_map_tmp;
    fc_CID.push_back(_g.CID); // 0. Store CID in order to check consistency
  }

  // 1. Construct queue in order to sort TreeSignatures
  priority_queue<TS_with_freq, vector<TS_with_freq>> Q;
  for(auto itr=TS_map.begin();itr!=TS_map.end();itr++)
    Q.push(TS_with_freq(itr->first,itr->second));

  // 2. Store frequency and determine feature names
  //    ... and output fringe-trees to fringe_file if it is specified (v02)
  int num_f_trees=0;
  unordered_map <TreeSignature, int> Ordered_TS_map;
  vector < TreeSignature > Ordered_TS;
  ofstream fout; 
  if(fringe_file!=""){
    fout.open(fringe_file);
    fout << "FID,(atom depth)-seq,Multiplicity seq" << endl;
  }

  
  while (!Q.empty()) {
    TS_with_freq T = Q.top();
    Ordered_TS_map[T.TS] = num_f_trees;
    Ordered_TS.push_back(T.TS);
    string fname = "FC_" + to_string(num_f_trees+1) + "_" + to_string(T.freq);
    fc_ftr_name.push_back(fname);
    if(fringe_file!=""){
      fout << num_f_trees+1 << ",";
      for (int i = 0; i < T.TS.delta.size(); i++)
	if(i%2==0)
	  fout << " " << map_int2atomic[T.TS.delta[i]];
	else
	  fout << " " << T.TS.delta[i];
      fout << ",";
      for (int i = 0; i < T.TS.mu.size(); i++)
	fout << " " << T.TS.mu[i];
      fout << endl;
    }
    num_f_trees++;
    Q.pop();
  }
  if(fringe_file!="")
    fout.close();
    
  // 3. Compute frequncy for each chemical compound
  for(auto CID:fc_CID){
    vector < double > v;
    v.resize(num_f_trees);
    for(int j=0;j<num_f_trees;j++)
      v[j] = 0.;
    for(int j=0;j<num_f_trees;j++){
      TreeSignature TS = Ordered_TS[j];
      v[j] = CID_TS_map[CID][TS];
    }
    fc_ftr_val.push_back(v);
  }

  
  
  /*
  if (sequential != 0) write_result(outputfilename, TS_map, total_fringe_tree_num);
  write_result_csv(outputfilename_csv, TS_map, total_fringe_tree_num);

  if (sequential == 0){
    ofstream outputfile(outputfilename, ios::out);
    outputfile.close();
  }
  */
}
