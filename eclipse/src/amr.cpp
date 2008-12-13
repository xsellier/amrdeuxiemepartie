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
 * Test the assertion, if true, exit the program with a message.
 */
void perror_and_exit_whenever(int assertion, string msg) {
	if (assertion) {
		perror(msg.c_str());
		exit(EXIT_FAILURE);
	}
}

/*
 * This function copy into a matrix all edges which are in the vector v
 * nb_node is the number of node and so the size of the matrix
 */
void fill_matrix(bool** matrix, vector<pair<int, int> > &v, int nb_node) {
	for (int i = 0; i < nb_node; ++i)
		for (int j = 0; j < nb_node; ++j)
			matrix[i][j] = false;

	//size of the vector and number of edges
	int v_size = v.size();

	for (int i = 0; i < v_size; ++i) {
		matrix[v.at(i).first][v.at(i).second] = true;
		matrix[v.at(i).second][v.at(i).first] = true;
	}
}

/*
 * Returns the highest node in the vector
 */
int max_node(vector<pair<int, int> > &v) {
	int max = 0;
	int size = v.size();

	for (int i = 0; i < size; ++i) {
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
void insert_edge(string edge, vector<pair<int, int> > &v) {
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
void edge_spliter(string edge, vector<pair<int, int> > &v) {
	size_t prec = 0;
	size_t found;
	size_t old_found = 0;

	found = edge.find_first_of("-");

	if (found != string::npos) {
		old_found = found;
		found = edge.find_first_of("-", found + 1);

		while (found != string::npos) {
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
int graph_init(char* filename, vector<pair<int, int> > &v) {
	ifstream file(filename, ios::in);
	string line;

	perror_and_exit_whenever(!file, "Erreur à l'ouverture du fichier");

	while (getline(file, line, '\n')) {
		size_t prec = 0;
		size_t found;

		found = line.find_first_of(" ");
		while (found != string::npos) {
			edge_spliter(line.substr(prec, found - prec), v);
			prec = found + 1;
			found = line.find_first_of(" ", prec);
		}
		edge_spliter(line.substr(prec), v);
	}

	file.close();
	return max_node(v);
}

void usage() {
	exit(EXIT_SUCCESS);
}

/*
 bool** tmp_matrix;

 tmp_matrix = new bool*[size];

 for (int i = 0; i < size; ++i)
 tmp_matrix[i] = new bool[size];

 */

void depth_first_search(bool** matrix, int size, int* cover) {
	int i, j, block;
	int nb_vertex = 0; // nb de sommet dans la couverture;
	for (i = 0; i < size; ++i)
		cover[i] = -1;
	i = 0;
	block = 0;
	j = i + 1;
	cover[0] = 0; // on initialise le père, la racine.
	do {
		if (matrix[i][j]) { 		// si le noeud i a un fils
			block=0;
			matrix[i][j]=0;
			if(cover[j]==-1){
				cover[j] = i;
				i = j;
				j=0; 				// important pour bien vérifier tous les fils
				nb_vertex++;
			} else {
				j++;				// pour ne pas modifier un noeud déjà parcouru
			}
		} else {
			j++;
			if (j >= size) {
				i = cover[i];
				j = i + 1;
				if(i == cover[i]) 	// dans le cas ou l'on bouclerait sur le même noeud
					block++;		// il y a des sommets isolés dans ce cas la.
			}
		}
	} while (nb_vertex < size && block <=2);
}

int main(int argc, char* argv[]) {
	if (argc < 2)
		usage();

	//vecteur servant à stoquer les arrêtes
	vector<pair<int, int> > edge;
	//vecteur servant à stoker les valeurs des variables renvoyées par minisat.
	vector<bool> variables;

	int size = graph_init(argv[1], edge) + 1;
	int father[size];
	int sons[size];
	int* cover;

	cover = new int[size];

	bool** matrix;

	matrix = new bool*[size];

	for (int i = 0; i < size; ++i)
		matrix[i] = new bool[size];

	fill_matrix(matrix, edge, size);

	depth_first_search(matrix, size, cover);

	for(int i = 0; i<size;++i)
		cout << "Le noeud " << i << " a pour père " << cover[i] << endl;

	exit(EXIT_SUCCESS);

}
