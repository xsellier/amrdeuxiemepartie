#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string>
#include <string.h>
#include <vector>
#include <sstream>

using namespace std;

/*
 * Test the assertion, if true, exit the programme with a message.
 */
void
perror_and_exit_whenever(int assertion, string msg)
{
  if (assertion)
    {
      perror(msg.c_str());
      exit(EXIT_FAILURE);
    }
}

/*
 * This function copy into a matrix all edges which are in the vector v
 * nb_node is the number of node and so the size of the matrix
 */
void
fill_matrix(bool** matrix, vector<pair<int, int> > &v, int nb_node)
{
  for (int i = 0; i < nb_node; ++i)
    for (int j = 0; j < nb_node; ++j)
      matrix[i][j] = false;

  //size of the vector and number of edges
  int v_size = v.size();

  for (int i = 0; i < v_size; ++i)
    {
      matrix[v.at(i).first][v.at(i).second] = true;
      matrix[v.at(i).second][v.at(i).first] = true;
    }
}

/*
 * Returns the highest node in the vector
 */
int
max_node(vector<pair<int, int> > &v)
{
  int max = 0;
  int size = v.size();

  for (int i = 0; i < size; ++i)
    {
      if (v[i].first > max)
        max = v[i].first;
      if (v[i].second > max)
        max = v[i].second;
    }

  return max;
}

/*
 * This function insert an isolated edge
 */
void
insert_isolated_edge(string edge, vector<pair<int, int> > &v)
{
  v.push_back(pair<int, int> (atoi(edge.c_str()), atoi(edge.c_str())));
}

/*
 * This function insert in the vector the edge contained in the string
 */
void
insert_edge(string edge, vector<pair<int, int> > &v)
{
  size_t pos = edge.find_first_of('-');
  int first_node = atoi(edge.substr(0, pos).c_str());
  int second_node = atoi(edge.substr(pos + 1, edge.length()).c_str());
  v.push_back(pair<int, int> (first_node, second_node));
}

/*
 * This function split this edges, the string edge contains one ou many edge,
 * it can contains 0-1 or 0-1-2-6. And insert edges in the vector with the function
 * insert_edge.
 */
void
edge_spliter(string edge, vector<pair<int, int> > &v)
{
  size_t prec = 0;
  size_t found;
  size_t old_found = 0;

  found = edge.find_first_of("-");

  if (found != string::npos)
    {
      old_found = found;
      found = edge.find_first_of("-", found + 1);

      while (found != string::npos)
        {
          insert_edge(edge.substr(prec, found - prec), v);
          prec = old_found + 1;
          old_found = found;
          found = edge.find_first_of("-", found + 1);
        }

      found = edge.substr(0, edge.find_last_of("-")).find_last_of("-");

      if (found == string::npos)
        prec = 0;
      else
        prec = found + 1;

      insert_edge(edge.substr(prec), v);
    }
  else
    insert_isolated_edge(edge, v);
}

/*
 * This function open the file witch contains the graph, and split it
 * in lines or in set of edges.
 */
int
graph_init(char* filename, vector<pair<int, int> > &v)
{
  ifstream file(filename, ios::in);
  string line;

  perror_and_exit_whenever(!file, "Erreur à l'ouverture du fichier");

  while (getline(file, line, '\n'))
    {
      size_t prec = 0;
      size_t found;

      found = line.find_first_of(" ");
      while (found != string::npos)
        {
          edge_spliter(line.substr(prec, found - prec), v);
          prec = found + 1;
          found = line.find_first_of(" ", prec);
        }
      edge_spliter(line.substr(prec), v);
    }

  file.close();
  return max_node(v);
}

/*
 * This function print a vector<pair<int,int> > in the standard output
 */
void
print_vector(vector<pair<int, int> > &v)
{
  int size = v.size();

  for (int i = 0; i < size; ++i)
    cout << v.at(i).first << " " << v.at(i).second << endl;
}

/*
 * This function print a matrix in the standard output
 */
void
print_matrix(bool** matrix, int size)
{
  for (int i = 0; i < size; ++i)
    {
      for (int j = 0; j < size; ++j)
        cout << matrix[i][j];

      cout << endl;
    }
}

/*
 * This function apply the reduction of k-color to sat, and write it in the
 * file flux
 */
void
k_color(bool** matrix, int size, int k, ofstream &file)
{

  int nb_clauses = 0;

  ostringstream clauses;

  clauses.clear();

  for (int i = 0; i < size; ++i)
    {

      //for every edge we check if two neighbour nodes don't have the same color
      for (int j = 0; j < size; ++j)
        if (matrix[i][j])
          for (int l = 0; l < k; ++l)
            {
              clauses << -(i * k + l) - 1 << " " << -(j * k + l) - 1 << " 0 \n";
              nb_clauses++;
            }

      //we check that every node have one color
      for (int l = 0; l < k; ++l)
        clauses << (i * k + l) + 1 << " ";

      clauses << "0 \n";
      nb_clauses++;

      //we check that two node can't have more than one color
      for (int l = 0; l < k; ++l)
        {
          for (int m = 0; m < l; ++m)
            {
              clauses << -(i * k + l) - 1 << " " << -(i * k + m) - 1 << " 0 \n";
              nb_clauses++;
            }

          for (int m = l + 1; m < k; ++m)
            {
              clauses << -(i * k + l) - 1 << " " << -(i * k + m) - 1 << " 0 \n";
              nb_clauses++;
            }
        }
    }

  file << "p cnf " << size * k << " " << nb_clauses << endl;
  clauses.flush();
  file << clauses.str() << endl;
  file.flush();
  file.close();
}

void
clique(bool **matrix, int size, int wanted_size, ofstream &file)
{
  ostringstream result;

  int position;
  int nb_clause = 0;

  //Each position is associated with at least one node
  for (position = 0; position < wanted_size; ++position)
    {
      for (int i = 0; i < size; ++i)
        result << position * size + i + 1 << " ";

      result << "0\n";
      nb_clause++;
    }

  //Two nodes can't got the same position
  for (position = 0; position < wanted_size; ++position)
    for (int i = 0; i < size; ++i)
      for (int j = i + 1; j < size; ++j)
        {
          result << -(position * size + i + 1) << " " << -(position * size + j
              + 1) << " 0\n";
          nb_clause++;
        }

  //One node can't have two different positions
  for (int i = 0; i < size; ++i)
    for (position = 0; position < wanted_size; ++position)
      for (int l = position + 1; l < wanted_size; ++l)
        {
          result << -(position * size + i + 1) << " " << -(l * size + i + 1)
              << " 0\n";
          nb_clause++;
        }

  //If two nodes don't got a adjacent edge, they can't be
  //both in the same clique.
  for (int i = 0; i < size; ++i)
    for (int j = i + 1; j < size; ++j)
      if (!matrix[i][j] && i != j)
        for (position = 0; position < wanted_size; ++position)
          for (int l = 0; l < wanted_size; ++l)
            {
              result << -(position * size + i + 1) << " "
                  << -(l * size + j + 1) << " 0\n";
              nb_clause++;
            }
  file << "p cnf " << size * wanted_size << " " << nb_clause << endl;
  file << result.str();;
  file.close();

}

/*
 * This function invert the boolean matrix, compute the complémentary
 * graph
 */
void
invert_matrix(bool **matrix, int size)
{
  for (int i = 0; i < size; ++i)
    for (int j = 0; j < size; ++j)
      if (i != j)
        matrix[i][j] = !(matrix[i][j]);
}

/*
 * This function compute the complemetary graph et check if there is a clique.
 */
void
independent_set(bool **matrix, int size, int wanted_size, ofstream &file)
{
  invert_matrix(matrix, size);

  clique(matrix, size, wanted_size, file);
}

/*
 * This function compute the complementary graph
 * and the biggest clique on this graph
 */
void
vertex_cover(bool **matrix, int size, int wanted_size, ofstream &file)
{
  invert_matrix(matrix, size);

  clique(matrix, size, size - wanted_size, file);
}

/*
 * This function is a common part from hamiltonian_path et hamiltonian_circuit
 */
void
hamitonian_common(ostringstream &result, bool **matrix, int size,
    ofstream &file)
{

  result << "p cnf " << size << " " << size << endl;

  //Each node of graph is at least at one position
  for (int i = 0; i < size; ++i)
    {
      for (int k = 0; k < size; ++k)
        result << i * size + k + 1 << " ";

      result << "0\n";
    }

  //Each position is associated with at least one node
  for (int i = 0; i < size; ++i)
    {
      for (int k = 0; k < size; ++k)
        result << k * size + i + 1 << " ";

      result << "0\n";
    }

  //Two nodes can't got the same position
  for (int i = 0; i < size; ++i)
    for (int j = 0; j < size - 1; ++j)
      for (int k = j + 1; k < size; ++k)
        {
          result << "-" << j * size + i + 1 << " -" << k * size + i + 1
              << " 0\n";
        }
}

//To code Hamiltonian path, we guess that
//the 1...n numbers represent positions for the first node,
//the n+1...2n numbers represent positions for the second,
//and etc....

void
hamiltonian_path(bool **matrix, int size, ofstream &file)
{

  ostringstream result;

  hamitonian_common(result, matrix, size, file);

  //If two nodes don't got a adjacent edge, they can't be
  //consecutive in hamiltonian path.
  for (int j = 0; j < size; ++j)
    for (int k = j; k < size; ++k)
      if (matrix[j][k] == false && j != k)
        for (int i = 0; i < size - 1; ++i)
          result << "-" << j * size + i + 1 << " -" << k * size + i + 2
              << " 0\n" << "-" << j * size + i + 2 << " -" << k * size + i + 1
              << " 0\n";

  file << result.str();
  file.close();
}

//To code Hamiltonian circuit, we take same algorithm as
//for Hamiltonian path, but we add clauses saying that
//the node which is at first position is too the last node.

void
hamiltonian_circuit(bool **matrix, int size, ofstream &file)
{

  ostringstream result;

  hamitonian_common(result, matrix, size, file);

  //If two nodes don't got a adjacent edge, they can't be
  //consecutive in hamiltonian path.
  //For Hamiltonian circuit, more, they can't be the first and the last.
  for (int j = 0; j < size; ++j)
    for (int k = j; k < size; ++k)
      if (matrix[j][k] == false && j != k)
        {
          for (int i = 0; i < size - 1; ++i)
            result << "-" << j * size + i + 1 << " -" << k * size + i + 2
                << " 0\n" << "-" << j * size + i + 2 << " -" << k * size + i
                + 1 << " 0\n";
          //For Hamiltonian circuit
          result << "-" << j * size + 1 << " -" << (k + 1) * size << " 0\n"
              << "-" << (j + 1) * size << " -" << k * size + 1 << " 0\n";
        }

  string res = result.str();

  file << res;
  file.close();
}

void
usage()
{
  cout << "premier problème k-colorabilité " << endl;
  cout << "$>amr <ficher de graph> 1 k" << endl;
  cout << endl;

  cout << "second problème circuit hamiltonien " << endl;
  cout << "$>amr <ficher de graph> 2 " << endl;
  cout << endl;

  cout << "troisième problème couverture de sommets " << endl;
  cout << "affiche une couverture de sommet de taille k si elle existe" << endl;
  cout << "$>amr <ficher de graph> 3 k" << endl;
  cout << endl;

  cout << "quatrième problème clique " << endl;
  cout << "affiche une clique de taille t si elle existe" << endl;
  cout << "$>amr <ficher de graph> 4 t" << endl;
  cout << endl;

  cout << "cinquième problème ensemble indépendant " << endl;
  cout << "affiche un ensemble indépendant de taille t si il existe" << endl;
  cout << "$>amr <ficher de graph> 5 t" << endl;
  cout << endl;

  cout << "bonus chemin hamiltonien " << endl;
  cout << "$>amr <ficher de graph> 6 " << endl;
  cout << endl;

  exit(EXIT_SUCCESS);
}

/*
 * Add a variable in the vector, true if it's positive and false if négative
 */
void
add_var(string text_var, vector<bool> &var)
{
  var.push_back(text_var[0] != '-');
}

/*
 * This functions opens the result file of minisat split and insert variables and their values
 * in the vector.
 */
void
get_var_result(vector<bool> &var, string result)
{
  ifstream file(result.c_str(), ios::in);

  string line;

  perror_and_exit_whenever(!file, "Erreur à l'ouverture du fichier");

  getline(file, line, '\n');

  //If not satisfiable we stop the function
  if (line.find("UNSAT") != string::npos)
    return;

  while (getline(file, line, '\n'))
    {
      size_t prec = 0;
      size_t found;

      found = line.find_first_of(" ");
      while (found != string::npos)
        {
          add_var(line.substr(prec, found - prec), var);
          prec = found + 1;
          found = line.find_first_of(" ", prec);
        }
    }

  file.close();
}

/*
 * This function call minisat, wait for the result and call get_var_result
 */
void
execute_minisat(string minisat, string result, vector<bool> &var)
{
  string cmd = "./minisat " + minisat + " " + result;

  system(cmd.c_str());
  get_var_result(var, result);
}

/*
 * This function read a vector of variables and gives the color of each node
 */
void
analyse_k_color(vector<bool> &var, int k)
{
  int size = var.size();

  if (size == 0)
    cout << "le graph n'est pas " << k << " coloriable" << endl;

  for (int i = 0; i < size; i = i + k)
    for (int j = i; j < i + k; ++j)
      if (var[j])
        cout << "le sommet " << (int) (i / k) << " a la couleur " << j - i
            << endl;

}

/*
 * This function read a vector of variables and tells which of them are in the biggest clique
 */
void
analyse_clique(vector<bool> &var, int wanted_size)
{
  int size = var.size();

  if (size == 0)
    cout << "le graph n'a pas de clique de taille " << wanted_size << endl;
  else
    {
      cout << "Voici une clique de taille " << wanted_size << " :" << endl;

      for (int i = 0; i < size; ++i)
        if (var[i])
          cout << i % (size / wanted_size) << " ";
    }
  cout << endl;
}

/*
 * This function read a vector of variables, and tells which of them are in
 * the biggest independant set
 */
void
analyse_independent_set(vector<bool> &var, int wanted_size)
{
  int size = var.size();

  if (size == 0)
    cout << "le graph n'a pas d'ensemble indépendant de taille " << wanted_size
        << endl;
  else
    {
      cout << "Voici un ensemble indépendant de taille " << wanted_size << " :"
          << endl;

      for (int i = 0; i < size; ++i)
        if (var[i])
          cout << i % (size / wanted_size) << " ";
    }
  cout << endl;
}

/*
 * This function is a common part of analyse_hamiltonian_path and analyse_hamiltonian_circuit.
 * It displays nodes in the correct order
 */
void
analyse_hamiltonian_common(vector<bool> &var, int nb_node, int size)
{
  int order[nb_node];

  for (int i = 0; i < size; i = i + nb_node)
    for (int j = i; j < i + nb_node; ++j)
      if (var[j])
        order[j - i] = (int) (i / nb_node);

  for (int i = 0; i < nb_node; ++i)
    cout << order[i] << " ";

  cout << endl;
}

/*
 * This function read a vector of variables and tells which of them are on an hamiltinian path
 */
void
analyse_hamiltonian_path(vector<bool> &var, int nb_node)
{
  int size = var.size();

  if (size == 0)
    cout << "le graph n'a pas de chemin hamiltonien" << endl;
  else
    {
      cout << "le chemin hamiltonien est : " << endl;
      analyse_hamiltonian_common(var, nb_node, size);
    }
}

/*
 * This function read a vector of variables and tells which of them are on an hamiltinian
 * circuit
 */
void
analyse_hamiltonian_circuit(vector<bool> &var, int nb_node)
{
  int size = var.size();

  if (size == 0)
    cout << "le graph n'a pas de circuit hamiltonien" << endl;
  else
    {
      cout << "le circuit hamiltonien est : " << endl;
      analyse_hamiltonian_common(var, nb_node, size);
    }
}

/*
 * This function read a vector of variables and tells which of them are
 *  a vertex_cover.
 */
void
anlyse_vertex_cover(vector<bool> &var, int nb_node, int wanted_size)
{
  int size = var.size();
  bool tab[nb_node];

  for (int i = 0; i < nb_node; ++i)
    tab[i] = true;

  if (size == 0)
    cout << "le graph n'a pas de couverture de sommets de taille "
        << wanted_size << endl;
  else
    {
      cout << "Voici une couverture de sommets de taille " << wanted_size
          << " :" << endl;

      for (int i = 0; i < size; ++i)
        if (var[i])
          tab[i % (size / (nb_node - wanted_size))] = false;

      for (int i = 0; i < nb_node; ++i)
        if (tab[i])
          cout << i << " ";
    }
  cout << endl;
}

int
main(int argc, char* argv[])
{

  if (argc < 3)
    usage();

  //fichier contenant le graph que l'on va passer à minisat.
  string minisat = "./minisat.txt";
  //fichier où minisat va remplir le résultat
  string result = "./result.txt";
  //vecteur servant à stoquer les arrêtes
  vector<pair<int, int> > edge;
  //vecteur servant à stoker les valeurs des variables renvoyées par minisat.
  vector<bool> variables;

  int size = graph_init(argv[1], edge) + 1;
  bool** matrix;

  matrix = new bool*[size];

  for (int i = 0; i < size; ++i)
    matrix[i] = new bool[size];

  fill_matrix(matrix, edge, size);

  ofstream file(minisat.c_str());

  int pb = atoi(argv[2]);

  switch (pb)
    {
  case 1:
    if (argc != 4)
      usage();

    k_color(matrix, size, atoi(argv[3]), file);
    execute_minisat(minisat, result, variables);
    analyse_k_color(variables, atoi(argv[3]));
    break;

  case 2:
    hamiltonian_circuit(matrix, size, file);
    execute_minisat(minisat, result, variables);
    analyse_hamiltonian_circuit(variables, size);
    break;

  case 3:
    if (argc != 4)
      usage();

    vertex_cover(matrix, size, atoi(argv[3]), file);
    execute_minisat(minisat, result, variables);
    anlyse_vertex_cover(variables, size, atoi(argv[3]));
    break;

  case 4:
    if (argc != 4)
      usage();

    clique(matrix, size, atoi(argv[3]), file);
    execute_minisat(minisat, result, variables);
    analyse_clique(variables, atoi(argv[3]));
    break;

  case 5:
    if (argc != 4)
      usage();

    independent_set(matrix, size, atoi(argv[3]), file);
    execute_minisat(minisat, result, variables);
    analyse_independent_set(variables, atoi(argv[3]));
    break;

  case 6:
    hamiltonian_path(matrix, size, file);
    execute_minisat(minisat, result, variables);
    analyse_hamiltonian_path(variables, size);
    break;
    }

  exit(EXIT_SUCCESS);
}

