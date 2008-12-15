#include <iostream>
#include <list>
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
 * Test the assertion, if true, exit the program with a message.
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
  //else isolate_edge => don't care
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

void
usage()
{
  exit(EXIT_SUCCESS);
}

void
depth_first_search(bool** matrix, int size, int* cover)
{
  int i, j, k;
  int nb_vertex = 1; // nb de sommet dans la couverture;
  for (i = 0; i < size; ++i)
    cover[i] = -1;
  i = 0;
  j = i + 1;
  cover[0] = 0; // on initialise le père, la racine.
  do
    {
      if (matrix[i][j])
        { // si le noeud i a un fils
          matrix[i][j] = 0;
          if (cover[j] == -1)
            {
              cover[j] = i;
              i = j;
              j = 0; // important pour bien vérifier tous les fils
              nb_vertex++;
            }
          else
            {
              j++; // pour ne pas modifier un noeud déjà parcouru
            }
        }
      else
        {
          j++;
          if (j >= size)
            {
              if (i != cover[i])
                { // le noeud a été totallement parcouru
                  i = cover[i]; // on remonte a son père
                  j = 0;
                }
              else
                { // dans le cas ou l'on bouclerait sur le même noeud
                  k = 0;
                  while (k < size)
                    {
                      if (cover[k] == -1)
                        { // on cherche un noeud non traité
                          nb_vertex++;
                          cover[k] = k;
                          i = k;
                          j = 0;
                          k = size; // pour casser la boucle
                        }
                      k++;
                    }
                }
            }
        }
    }
  while (nb_vertex < size);
}

void
tree_cover(int size, int tree[], bool* cover)
{
  for (int i = 0; i < size; ++i)
    // initialisation
    cover[i] = false;
  for (int i = 0; i < size; ++i)
    cover[tree[i]] = true; // met a 1 tous les noeuds faisant partis de la couverture
}

void
tree_cover(int size, int tree[], list<int> cover)
{
  for (int i = 0; i < size; ++i)
    cover.push_front(tree[i]);// met les noeuds faisant partie de la couverture dans la liste
}

void
tree_vc(int father[], int nb_sons[], list<int> &leaf, bool vertex_cover[])
{
  //on prend la première feuille de libre.
  int i = leaf.front();

  //on considère que 0 est toujours la racine car il faut bien en choisir un
  if (i == 0 || leaf.size() <= 0)
    return;

  int j = father[i];
  int k = father[j];
  // je retire la feuille pour ne pas la retraiter
  leaf.pop_front();

  vertex_cover[j] = true;

  //on casse l'arête entre i et j
  father[i] = -1;
  //on casse l'arête entre j et k
  father[j] = -1;

  //si j n'a pas de père
  if (k != -1)
    {
      nb_sons[k]--;
      //si k devient une feuille
      if (nb_sons[k] == 0)
        leaf.push_front(k);
    }

  //on rappelle la fonction
  tree_vc(father, nb_sons, leaf, vertex_cover);
}

void
init_tree_vc(int father[], int nb_sons[], bool vertex_cover[], list<int> &leaf,
    int size, vector<pair<int, int> > edge)
{
  for (int i = 0; i < size; ++i)
    {
      father[i] = -1;
      nb_sons[i] = 0;
      vertex_cover[i] = false;
    }

  int v_size = edge.size();

  for (int i = 0; i < v_size; ++i)
    {
      father[edge.at(i).second] = edge.at(i).first;
      nb_sons[edge.at(i).first]++;
    }

  for (int i = 0; i < size; ++i)
    if (nb_sons[i] == 0)
      leaf.push_front(i);
}

int
main(int argc, char* argv[])
{
  if (argc < 3)
    usage();

  //vecteur servant à stoquer les arrêtes
  vector<pair<int, int> > edge;
  //vecteur servant à stoker les valeurs des variables renvoyées par minisat.
  vector<bool> variables;

  int size = graph_init(argv[1], edge) + 1;

  list<int> leaf;
  list<int> vc;
  list<int>::iterator vc_iterator;

  int pb = atoi(argv[2]);

  switch (pb)
    {
  case 1:
    {
      int father[size];
      int sons[size];
      bool vertex_cover[size];

      init_tree_vc(father, sons, vertex_cover, leaf, size, edge);
      tree_vc(father, sons, leaf, vertex_cover);

      for (int i = 0; i < size; ++i)
        if (vertex_cover[i])
          vc.push_front(i);

      cout << "Les sommets suivants sont dans la couverture : ";
      for (vc_iterator = vc.begin(); vc_iterator != vc.end(); ++vc_iterator)
        cout << *vc_iterator << " ";

      cout << endl;

      break;
    }
  case 2:
    {
      int* tree;
      tree = new int[size];
      bool* cover;
      cover = new bool[size];
      bool** matrix;
      matrix = new bool*[size];

      for (int i = 0; i < size; ++i)
        matrix[i] = new bool[size];

      fill_matrix(matrix, edge, size);
      depth_first_search(matrix, size, tree);
      tree_cover(size, tree, cover);

      for (int i = 0; i < size; ++i)
        cout << "Le noeud " << i << " a pour père " << tree[i] << endl;

      for (int i = 0; i < size; ++i)
        {
          if (cover[i])
            cout << "Le noeud " << i << " fait partie de la couverture " << endl;
        }
      break;
    }
    }

  exit(EXIT_SUCCESS);
}
