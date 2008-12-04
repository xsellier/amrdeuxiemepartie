/* gcc gengraph.c -o gengraph ; strip gengraph */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARG(s) (strcmp(argv[i],s)==0)
#define DIMAX 256 /* nb maximum de param�tres d'un graphe */

int N;           /* N=nb de sommets du graphes avant suppression */
int NF;          /* nb de sommets final du graphes */
int *V;          /* �tiquette des sommets, en principe V[i]=i */
int SHIFT=0;     /* d�but de la num�rotation des sommets */
int PARAM[DIMAX];/* liste des param�tres du graphe */
double DELE=0.0; /* proba de supprimer une ar�tes */
double DELV=0.0; /* proba de supprimer un sommet */
int PERMUTE=0;   /* vrai ssi -permute */
int NOT=0;       /* vrai ssi -not */
int FORMAT=0;    /* type de la sortie, par d�faut 0=standard */
int WIDTH=10;    /* nb maximum d'ar�tes/sommets isol�s affich�s par ligne */
int LIGNE=0;     /* compteur d'ar�tes/sommets isol�s par ligne */
int ARGC;        /* variable globale pour argc */
char **ARGV;     /* variable globale pour argv */
int **MAT;       /* pour le format de matrice d'ajacence */
int **REP;       /* associe � chaque sommet une repr�sentation sous forme
                    de tableau d'entiers. Sert pour les repr�sentations
                    implicites de graphes, les arbres, les graphes
                    d'intersections, etc. */

void Erreur(int error){
  char *s;
  switch(error){
    case 1: s="Erreur : nombre d'argument insuffisant."; break;
    case 2: s="Erreur : argument inconnu."; break;
    case 3: s="Erreur : espace m�moire insuffisant."; break;
    case 4: s="Erreur : nombre trop grand de param�tres."; break;
    case 5: s="Erreur : format d'affichage inconnu."; break;
    default: s="Erreur : code d'erreur inconnue.";
  }
  printf("%s\n",s);
  exit(0);
}


int *AllocTab(int n){
  /*
    Cr�e un tableau de n entiers
  */
  int *p;
  p=malloc(n*sizeof(int));
  if(p==NULL) Erreur(3);
  return p;
}


int **AllocMat(int n,int t){
  /*
    Cr�er une matrice de n x t enti�re, c'est-�-dire de n tableaux de
    t entiers.
  */
  int **X,k;
  X=malloc(n*sizeof(int*));
  if(X==NULL) Erreur(3);  
  for(k=0;k<n;k++) X[k]=AllocTab(t);
  return X;
}


void FreeMat(int **X,int n){
  /*
    Lib�re le tableau X de n sous-tableaux d'entiers
  */
  int k;
  for(k=0;k<n;k++) free(X[k]);
  free(X);
}


/***********************************

           GRAPHES DE BASE
               (d�but)

***********************************/

/*
  Les fonctions d'adjacences adj(i,j) doivent avoir les fonctionalit�s
  suivantes :

  1. adj(-1,j): inialise l'adjacence (appel�e avant)
  2. adj(i,-1): finalise l'adjacence (appel�e apr�s)
  3. adj(i,j) pour i,j>=0, retourne 1 ssi i est adjacent � j

 */

int prime(int i,int j){
  if(j<0) return 0;
  if(i<0){ N=PARAM[0]; return N; }
  return (i>1)? ((j%i)==0) : 0;
}

int ring(int i,int j){
  int k;
  if(j<0) return 0;
  if(i<0){ N=PARAM[0]; return N; }

  for(k=0;k<PARAM[1];k++)
    if((j==((i+PARAM[2+k])%N))||(i==((j+PARAM[2+k])%N))) return 1;

  return 0;
}

int WRAP[DIMAX]; /* WRAP[k]=1 ssi la dimension k est "torique" */

int grid(int i,int j){
  int x,y,k,z,d,p,h,b;

  if(j<0) return 0;
  if(i<0){
    N=1;
    for(k=0;k<PARAM[0];k++){
      p=PARAM[k+1];
      WRAP[k]=(p<0);
      p=abs(p);
      PARAM[k+1]=p;
      N *= p;
    }
    return N;
    }

  d=PARAM[0];
  z=h=k=b=0;

  while((k<d)&&(b<2)&&(h<2)){
    p=PARAM[k+1];
    x=i%p;
    y=j%p;
    h=abs(x-y);
    if(h==0) z++;
    if((h>1)&&(WRAP[k])&&(((x==0)&&(y==p-1))||((y==0)&&(x==p-1)))) h=1;
    if(h==1) b++;
    i /= p;
    j /= p;
    k++;
  }

  return (b==1)&&(z==(d-1));
}

int *TREE;
/* 
   Tableau global pour les arbres (sert aussi pour les outerplanars).

   Ex: Arbre avec N=5 sommets, soit n=4 ar�tes.
   On tire un tableau B=[0,1,2,1] (de taille n et dont la somme fait n),
   ce qui donne le mot de Dick "01011010". On d�cale le mot sur un minimum
   (r=1 dans B), d'o� le mot "10110100":

   Mot     3   4        Arbre   0
          / \ / \              / \
     1   2   2   2            1   2
    / \ /         \              / \
   0   0           0            3   4

   TREE=[0,1,0,2,3,2,4,2,0] (taille 2N-1): c'est la liste des sommets
   rencontr�s sur une marche le long du mot.
   Adjacence: i-j ssi on trouve ...i,j... dans le tableau TREE.
   Si i<j, alors pas montant, sinon pas descendant.
   Remarque: 0 est toujours la racine, et le sommet N-1 est la derni�re feuille.
*/


int tree(int i,int j){
  int n,r,k,s,m,t,h;
  int *B,*H; /* tableaux auxiliaires */

  if(j<0){
    free(TREE);
    FreeMat(REP,N);
    return 0;
  }

  if(i<0){
    /* Initialise N, et les tableaux TREE et REP.
       REP[i]=p�re de i. La racine est le sommet i=0. */
    N=PARAM[0];
    if(N<2) return N; /* fin si moins de 2 sommets */

    TREE=AllocTab(2*N-1); /* il faut N>1 */
    REP=AllocMat(N,1); /* pour repr�sentation implicite */
    B=AllocTab(N-1);
    H=AllocTab(N+1); /* hauteurs possibles: 0...N */
    n=N-1;

    /* remplit B de valeurs dont la somme fait n */

    for(k=0;k<n;k++) B[k]=0;
    for(k=0;k<n;k++) B[rand()%n]++;

    /* cherche la racine r, et donc la hauteur minimum */

    r=-1;
    s=h=0;
    for(k=0;k<n;k++){
      s += B[k]-1;
      if(s<h) { h=s; r=k; }
    }
    r=(r+1)%n;

    /*
      Remplissage de TREE:
      On parcours le mot � partir de la racine r.
      H[h]=dernier sommet rencontr� de hauteur h
      h=hauteur du dernier sommet de H
      s=num�ro du prochain sommet
      t=index libre du tableau TREE
    */
    TREE[0]=H[0]=h=0; /* on traite le sommet 0 */
    s=t=1;

    for(k=0;k<n;k++){
      m=B[(k+r)%n];
      for(j=0;j<m;j++){
	TREE[t++]=s;
	H[++h]=s++;
      }
      TREE[t++]=H[--h];
    }

    free(B); /* ne garde que le tableau TREE */
    free(H);

    /* Pour la repr�sentation implicite.
       On parcours le tableau TREE. Soient r,s deux valeurs cons�cutives
       dans TREE. Si r=N-1, alors tous les sommets ont �t� parcourus.
       Si s>r, alors c'est un pas montant, et donc REP[s][0]=r. */
    REP[r=k=0][0]=-1; /* pas de p�re pour la racine */
    while(r<N-1){
      s=TREE[++k];
      if(s>r) REP[s][0]=r;
      r=s;
    }

    return N;
  }

  /*
    Test d'adjacence i-j ?
    j p�re de i, ou bien, i p�re de j.
  */
  return ( (REP[i][0]==j) || (REP[j][0]==i) );

}


int outerplanar(int i,int j){
  int k,r,s,m,z;
  int **P; /* pointeur auxiliaire */

  if(j<0){
    tree(i,j); /* Lib�re TREE et REP */
    return 0;
  }

  if(i<0){
    tree(i,j); /* Initialise N, TREE et REP */
    if(N<2) return N; /* fin si moins de 2 sommets */

    /* Il faut redimensioner le tableau REP:
       REP[k][0]=p�re de k dans l'arbre.
       REP[k][1]=p�re2 de k. */

    P=AllocMat(N,2); /* alloue un nouveau tableau, plus large */
    for(k=0;k<N;k++) /* copie REP dans P */
      P[k][0]=REP[k][0];
    FreeMat(REP,N);  /* lib�re l'ancien tableau REP */
    REP=P;           /* REP vaut maintenant P */

    /* On calcule REP[k][1]:
       On parcoure TREE puis:
       - si on descend on rep�re z, l'indice de la feuille.
       - si on remonte, on met � jour les p�res2 des sommets de la
         branche pr�c�dente: ce sont les sommets de TREE dont l'indice
         va de z � k-2 (k et k-1 sont impossibles). */

    for(k=0;k<N;k++) REP[k][1]=-1; /* par d�faut, pas de p�re2 */
    r=k=z=m=0; /* r-s sont deux valeurs cons�cutives dans TREE */
               /* m=1 ssi mont�e */

    while(r<N-1){ /* le dernier sommet est la derni�re feuille */
      s=TREE[++k];
      if((m==1)&&(s<r)){ /* on commence � descendre */
	m=1-m;
	z=k-1;
      }
      if((m==0)&&(s>r)){ /* on commence � monter */
	m=1-m;
	while(z<k-1) REP[TREE[z++]][1]=(rand()%2)?s:-1;
      }
      r=s;
    }

    return N;
  }

  /*
    Test d'adjacence i-j ?
    tree(i,j) ou bien j p�re2 de i, ou bien i p�re2 de j.
  */
  return ( tree(i,j) || (REP[i][1]==j) || (REP[j][1]==i) );
  }

int permutation(int i,int j){
  int k,x,m;

  if(j<0){ FreeMat(REP,N); return 0; }
  if(i<0){
    N=PARAM[0];
    REP=AllocMat(N,1);

    /* g�n�re dans une permutation al�atoire de [0,N[ */
    for(k=0;k<N;k++) REP[k][0]=k;
    for(k=0;k<N;k++){
      m=k+(rand()%(N-k));
      x=REP[m][0]; REP[m][0]=REP[k][0]; REP[k][0]=x;
    }

    return N;
  }

  return ((i-REP[i][0])*(j-REP[j][0])<0);
}


int interval(int i,int j){
  int k,x,m;

  if(j<0){ FreeMat(REP,N); return 0; }
  if(i<0){
    N=PARAM[0];
    m=2*N;

    /* g�n�re un intervalle REP[k] pour k, [a,b] dans [0,2N[ avec a<=b */
    REP=AllocMat(N,2);
    for(k=0;k<N;k++){
      x=rand()%m;
      REP[k][0]=x;
      REP[k][1]=x+rand()%(m-x);
    }
    return N;
  }

  return ( ((REP[i][0]<=REP[j][0])&&(REP[j][1]<=REP[i][1])) || ((REP[j][0]<=REP[i][0])&&(REP[i][1]<=REP[j][1])) );
}


int sat(int i,int j){
  /*
    [0,2n[: les variables positives et n�gatives
    [2n+i*k,2n+(i+1)*k[: clause num�ro i (i=0 ... PARAM[1])
   */
  int n,k;

  if(j<0) return 0;
  if(i<0){
    n=2*PARAM[0];
    N=n+PARAM[1]*PARAM[2];
    REP=AllocMat(N,1);

    /* chaque sommet-clause est connect� � une variable (positive ou n�gative) */
    for(k=n;k<N;k++) REP[k][0]=rand()%n;

    return N;
  }

  if(i>j) { k=i; i=j; j=k; }
  /* maintenant i<j */
  n=2*PARAM[0];
  k=PARAM[2];

  if(j<n) return ((j==i+1)&&(j%2)); /* i-j et j impaire */
  if(i>=n) return (j-i<=k); /* dans la m�me clique ? */
  return (REP[j][0]==i);
}


int kneser(int i,int j){
  int *B,n,k,r,v,x,y;

  if(j<0) { FreeMat(REP,N); return 0; }
  if(i<0){
    n=PARAM[0];
    k=PARAM[1];
    r=PARAM[2];

    /*
      Calcule l'entier N={n choose k}.  Cela est n�cessaire pour
      allouer le tableau REP. Et malheureusement, on ne peut pas le
      faire � la vol�e au moment o� l'on g�n�re les ensembles
      repr�sentant les sommets, � part faire des realloc() mais sera
      au final bien plus lent que de calculer N.

      L'algorithme utilis� ici est en O(k). Il n'utilise que des
      multiplications et divisions enti�res sur des entiers en O(N).
      L'algorithme issu du Triangle de Pascal est lui en O(nk),
      utilise un espace (tableaux d'entiers en N) en O(k), mais
      n'utilise que des additions sur des entiers en O(N).

      Principe: N = n x (n-1) x ... x (n-k+1) / k x (k-1) x ... x 1

      On r�ecrit en (((((n/1) x (n-1)) / 2) x (n-2)) / 3) ...
      c'est-�-dire en multipliant le plus grand num�rateur et en
      divisant par le plus petit num�rateur. Chaque terme est ainsi un
      certain binomial, et donc un entier.
    */

    N=x=n;
    for(y=2;y<=k;y++){
      x--;
      N=(N*x)/y;
    }

    REP=AllocMat(N,k); /* on conna�t N */

    /*
      Calcule tous les sous-ensembles � k �l�ments. L'id�e est de
      maintenir une sorte de compteur B qui repr�sente le
      sous-ensemble courant et qui va passer par tous les
      sous-ensembles possibles � k �l�ments. Au d�part on part du plus
      "petit" ensemble, B=[0 1 2 ... k-1], les �l�ments �tant rang�s
      dans l'ordre croissant. La strat�gie pour "incr�menter" B,
      c'est-�-dire pour passer au sous-ensemble suivant, est la
      suivante : on essaye d'incr�menter le plus petit �l�ment de B,
      donc B[0], tout en restant strictement inf�rieur � l'�l�ment
      suivant B[1]. Si cela n'est pas possible, on r�initialise B[0] �
      sa plus petite valeur, 0 ici, et on essaye d'incr�menter le
      second plus petit �l�ment de B, soit B[1]. On ar�te si on a pas
      pu incr�menter B[k-1] sans d�passer la sentiennelle B[k]=n. D�s
      qu'on a trouv� un �l�ment B[i] que l'on peut incr�menter on le
      fait, et on a donc g�n�r� le nouvel ensemble B. On recommence
      ensuite la m�thode en essayant d'incr�menter le plus petit
      �lement, etc. A priori l'algorithme est en O(N*k), mais de
      mani�re amortie c'est sans doute moins, on incr�mente moins
      souvent B[j] que B[i] si j>i. Ceci dit on paye de toute fa�on
      une copie du tableau REP=B de taille k pour chacun des N
      sous-ensembles. Cela reste en O(N*k).
     */

    B=AllocTab(k+1);
    for(x=0;x<k;x++) B[x]=x;
    B[k]=n; /* sentinelle importante */

    v=0; /* v=sommet courant */
    goto jump1; /* la premi�re fois REP[0]=B */

    while(x<k){
      if(++B[x]<B[x+1]){ /* cas o� B est correct */
      jump1:
	x=0;
	for(y=0;y<k;y++) REP[v][y]=B[y];
	v++;
      }
      else B[x]=x++; /* B n'est pas correct */
    }

    free(B);
    return N;
  }

  /*
    Calcule si l'intersection poss�de au plus r �l�ments. L'algorithme
    ici est en O(k) en utilisant le fait que les �l�ments de REP sont
    rang�s dans un ordre croissant.
   */

  v=0; /* v=nb d'�lements commun */
  x=y=0; /* indices pour i et j */
  k=PARAM[1];
  r=PARAM[2];

  while((v<=r)&&(x<k)&&(y<k)){
    if(REP[i][x]==REP[j][y]) {
      v++;
      x++;
      y++;
    }
    else
      if(REP[i][x]<REP[j][y]) x++; else y++;
  }

  return (v<=r);
}


int kout(int i,int j){
  /*
    REP[i]=tableau des voisins de i. Si REP[i][j]<0, alors c'est la
    fin de la liste.  Attention ! Si i<j, alors REP[i] ne peut pas
    contenir le sommet j (qui a �t� cr�e apr�s i). Pour le test
    d'adjacence, il faut donc tester si i est dans REP[j], et pas le
    contraire !
   */
  int n,k,r,d,x,y,z;
  int *T;

  if(j<0){
    FreeMat(REP,PARAM[1]);
    return 0;
  }

  k=PARAM[1];

  if(i<0){
    N=PARAM[0];
    REP=AllocMat(N,k);
    T=AllocTab(N);

    REP[0][0]=-1;     /* le sommet 0 est seul !*/
    for(x=1;x<N;x++){ /* x=prochain sommet � rajouter */
      r=(x<k)?x:k;    /* le degr� de x sera au plus r=min{x,k}>0 */
      d=rand()%(r+1); /* choisir un degr� d pour x: d=[0,...,r] */
      for(y=0;y<x;y++) T[y]=y; /* tableau des voisins possibles */
      r=x;              /* r-1=index du dernier �l�ment de T */
      for(y=0;y<d;y++){ /* choisir d voisins dans T */
	z=rand()%r;     /* on choisit un voisin parmi ceux qui restent */
        REP[x][y]=T[z]; /* le y-�me voisin de x est T[z] */
        T[z]=T[--r];    /* enl�ve T[z], met le dernier �l�ment de T � sa place */
      }
      if(d<k) REP[x][d]=-1; /* arr�te la liste des voisins de x */
    }

    return N;
  }

  if(i>j) { x=i; i=j; j=x; }
  /* maintenant i<j, donc j a �t� cr�e apr�s i */
  
  for(y=0;y<k;y++){
    if(REP[j][y]==i) return 1;
    if(REP[j][y]<0) return 0;
  }
  return 0;
}


/***********************************

           GRAPHES DE BASE
               (fin)

***********************************/

int InitVertex(int n){
/*
** Remplit le tableau V[] donnant l'�tiquette du sommet i et supprime les
** sommet suivant la valeur DELV. Utilise aussi SHIFT.
** Si PERMUTE est vrai V[] est remplit d'une permutation al�atoire de
** SHIFT+[0,n[. V[i]=-1 signifie que i a �t� supprim� (DELV).
** La fonction retourne le nombre de sommets final du graphe, c'est-�-dire
** d'�tiquette >=0. Si k sommets ont �t� supprim�s, alors les valeurs
** de V sont dans SHIFT+[0,n-k[
*/

  int i,j,k,seuil,r=0; /* r=n-(nb de sommets supprim�s) */

  seuil=(double)DELV*(double)RAND_MAX;

  for(i=0;i<n;i++)
    if(rand()<seuil) V[i]=-1; else V[i]=r++;

  if(PERMUTE)
    for(i=0;i<n;i++){
      j=i+(rand()%(n-i));
      k=V[j]; V[j]=V[i]; V[i]=k;
    }

  if(SHIFT>0) for(i=0;i<n;i++) if(V[i]>=0) V[i] += SHIFT;

  return r;
}


int LAST=-1;     /* extr�mit� de la derni�re ar�te affich�e */

void Out(int i,int j){
  /*
    Affiche l'ar�te i-j suivant le format FORMAT.
    Si i<0 et j<0, alors la fonction Out() est initialis�e.
    Si i<0, alors c'est la terminaison de la fonction Out()
    Si j<0, alors affiche i seul (sommet isol�).
   */
  char *s;
  int x,y;

  if((i<0)&&(j<0)){ /* initialise la fonction */
    switch(FORMAT){
    case 0: return;

    case 1:
    case 2:
    case 3:
      MAT=AllocMat(NF,NF); /* r�serve la matrice */
      for(x=0;x<NF;x++)    /* remplit la matrice de 0 */
	for(y=0;y<NF;y++)
	  MAT[x][y]=0;
      return;

    default: Erreur(5);
    }
  }

  if(i<0){ /* termine la fonction */
    switch(FORMAT){
    case 0:
      if(LIGNE>0) printf("\n"); /* saut de ligne si fini avant la fin de ligne */
      return;

    case 1:
      for(x=0;x<NF;x++){
	for(y=0;y<NF;y++) printf("%i",MAT[x][y]);
	printf("\n");
      }
      FreeMat(MAT,NF);     /* lib�re la matrice */
      return;

    case 2:
      for(x=0;x<NF;x++){
	for(y=0;y<=x;y++) printf("%i",MAT[x][y]);
	printf("\n");
      }
      FreeMat(MAT,NF);     /* lib�re la matrice */
      return;

    case 3:
      for(x=0;x<NF;x++){
	printf("%i:",x);
	for(y=0;y<NF;y++)
	  if(MAT[x][y]) printf(" %i",y);
	printf("\n");
      }
      FreeMat(MAT,NF);     /* lib�re la matrice */
      return;

    default: Erreur(5);
    }
  }

  /* affichage de "i-j", "i" ou "-j" */

  switch(FORMAT){

  case 0:
    if(j<0) LAST=-1; /* sommet isol�, donc LAST sera diff�rent de i */
    if(i!=LAST) printf("%s%i",(LIGNE==0)?"":" ",V[i]);
    LAST=j;   /* si i=LAST, alors affiche ...-j. Si j<0 alors LAST<0 */
    if(j>=0) printf("-%i",V[j]);
    LIGNE++;
    if(LIGNE==WIDTH){
      LIGNE=0;
      LAST=-1;
      printf("\n");
    }
    return;

  case 1: case 2: case 3:
    if(j>=0) MAT[V[i]][V[j]]=MAT[V[j]][V[i]]=1;
    return;

  default: Erreur(5);
  }
}


void Help(void){
  /*
    Affiche l'aide en ligne
  */

  char s[256]="sed -n '/*[#] ###/,/### #/p' ";
  strcat(strcat(s,ARGV[0]),".c");
  strcat(s," | sed 's/\\/\\*[#] ###//g'");
  strcat(s," | sed 's/### [#]\\*\\///g' | more");
  system(s);
  exit(0);
}


char *NextArg(int *i){
  /*
    Retourne argv[i] s'il existe, puis incr�mente i
  */

  if((*i)==ARGC) Erreur(1);
  return ARGV[(*i)++];
}

/***********************************

               MAIN

***********************************/


int main(int argc, char *argv[]){

  int (*adj)(int i,int j);
  int i,j,k,seuil,*inc;
  char *s;
  
  ARGC=argc;
  ARGV=argv;

  if(argc==1) Help(); /* aide si aucun argument */

  adj=ring; PARAM[0]=0; /* valeurs par d�faut */

  /* analyse de la ligne de commande */

  i=1; seuil=getpid();
  while(i<argc){
    if ARG("-help") Help();

    j=i;
    if ARG("-seed"){ i++;
      seuil=atoi(NextArg(&i));
      goto fin;
    }
    if ARG("-width"){ i++;
      WIDTH=atoi(NextArg(&i));
      goto fin;
    }
    if ARG("-shift"){ i++;
      SHIFT=atoi(NextArg(&i));
      goto fin;
    }
     if ARG("-permute"){ i++;
      PERMUTE=1;
      goto fin;
    }
    if ARG("-not"){ i++;
      NOT=1-NOT;
      goto fin;
    }
    if ARG("-delv"){ i++;
      DELV=atof(NextArg(&i));
      goto fin;
    }
    if ARG("-dele"){ i++;
      DELE=atof(NextArg(&i));
      goto fin;
    }
    if ARG("-format"){ i++;
      s=NextArg(&i);
      if(strcmp(s,"standard")==0) goto fin;
      if(strcmp(s,"matrix")==0)   FORMAT=1;
      if(strcmp(s,"smatrix")==0)  FORMAT=2;
      if(strcmp(s,"list")==0)     FORMAT=3;
      if(FORMAT==0) Erreur(5);
      goto fin;
    }
    if ARG("path"){ i++;
      adj=grid;
      PARAM[0]=1;
      PARAM[1]=atoi(NextArg(&i));
      goto fin;
    }
    if ARG("tree"){ i++;
      adj=tree;
      PARAM[0]=atoi(NextArg(&i));
      goto fin;
    }
    if ARG("outerplanar"){ i++;
      adj=outerplanar;
      PARAM[0]=atoi(NextArg(&i));
      goto fin;
    }
    if ARG("prime"){ i++;
      adj=prime;
      PARAM[0]=atoi(NextArg(&i));
      goto fin;
    }
    if ARG("hypercube"){ i++;
      adj=grid;
      PARAM[0]=atoi(NextArg(&i));
      if(PARAM[0]+1>DIMAX) Erreur(4);
      for(k=0;k<PARAM[0];k++) PARAM[k+1]=2;
      goto fin;
    }
    if ARG("torus"){ i++;
      adj=grid;
      PARAM[0]=2;
      PARAM[1]=-atoi(NextArg(&i));
      PARAM[2]=-atoi(NextArg(&i));
      goto fin;
    }
    if ARG("mesh"){ i++;
      adj=grid;
      PARAM[0]=2;
      PARAM[1]=atoi(NextArg(&i));
      PARAM[2]=atoi(NextArg(&i));
      goto fin;
    }
    if ARG("sat"){ i++;
      adj=sat;
      PARAM[0]=atoi(NextArg(&i));
      PARAM[1]=atoi(NextArg(&i));
      PARAM[2]=atoi(NextArg(&i));
      goto fin;
    }
    if ARG("ring"){ i++;
      adj=ring;
      PARAM[0]=atoi(NextArg(&i));
      PARAM[1]=atoi(NextArg(&i));
      if(PARAM[1]+2>DIMAX) Erreur(4);
      for(k=0;k<PARAM[1];k++)
	PARAM[k+2]=atoi(NextArg(&i));
      goto fin;
    }
    if ARG("grid"){ i++;
      adj=grid;
      PARAM[0]=atoi(NextArg(&i));
      if(PARAM[0]+1>DIMAX) Erreur(4);
      for(k=0;k<PARAM[0];k++)
	PARAM[k+1]=atoi(NextArg(&i));
      goto fin;
    }
    if ARG("cycle"){ i++;
      adj=ring;
      PARAM[0]=atoi(NextArg(&i));
      PARAM[1]=PARAM[2]=1;
      goto fin;
    }
    if ARG("stable"){ i++;
      adj=ring;
      PARAM[0]=atoi(NextArg(&i));
      PARAM[1]=0;
      goto fin;
    }
    if ARG("clique"){ i++;
      adj=ring;
      NOT=1-NOT;
      PARAM[0]=atoi(NextArg(&i));
      PARAM[1]=0;
      goto fin;
    }
    if ARG("random"){ i++;
      adj=ring;
      NOT=1-NOT;
      PARAM[0]=atoi(NextArg(&i));
      DELE=1.0-atof(NextArg(&i));
      PARAM[1]=0;
      goto fin;
    }
    if ARG("interval"){ i++;
      adj=interval;
      PARAM[0]=atoi(NextArg(&i));
      goto fin;
    }
    if ARG("permutation"){ i++;
      adj=permutation;
      PARAM[0]=atoi(NextArg(&i));
      goto fin;
    }
    if ARG("petersen"){ i++;
      adj=kneser;
      PARAM[0]=5;
      PARAM[1]=2;
      PARAM[2]=0;
      goto fin;
    }
    if ARG("kneser"){ i++;
      adj=kneser;
      PARAM[0]=atoi(NextArg(&i));
      PARAM[1]=atoi(NextArg(&i));
      PARAM[2]=atoi(NextArg(&i));
      if(PARAM[1]>PARAM[0]/2) PARAM[1]=PARAM[0]-PARAM[1];
      goto fin;
    }
    if ARG("kout"){ i++;
      adj=kout;
      PARAM[0]=atoi(NextArg(&i));
      PARAM[1]=atoi(NextArg(&i));
      if(PARAM[1]>=PARAM[0]-1) PARAM[1]=PARAM[0]-1;
      goto fin;
    }
  fin:
    if(j==i) Erreur(2); /* option non trouv�e */
  }

  srand(seuil); /* initialise le g�n�rateur al�atoire */
  adj(-1,0);    /* initialisation du graphe, calcul N avant
                   suppression des sommets */

  V=AllocTab(N);
  NF=InitVertex(N); /* initialise NF et les �tiquettes V[] des sommets */
  seuil=(double)(1.0-DELE)*(double)RAND_MAX;
  inc=AllocTab(N); /* inc[i]=1 ssi i poss�de un voisin, 0 si sommet isol� */
  for(i=0;i<N;i++) inc[i]=0;

  /* g�n�re les adjacences i-j en tenant compte
     des sommets isol�s et des sommets supprim�s */

  Out(-1,-1); /* initialise le format d'affichage */

  for(i=0;i<N;i++)
    if(V[i]>=0){
      for(j=i+1;j<N;j++)
	if(V[j]>=0)
	  if((rand()<seuil)&&(adj(i,j)^NOT)){
	    inc[i]=inc[j]=1;
	    Out(i,j);
	  }
      if(inc[i]==0) Out(i,-1);
    }

  Out(-1,0); /* fin d'affichage */
  adj(0,-1); /* lib�re �ventuellement la m�moire utilis�e pour adj() */

  free(V);
  free(inc);
  return 0;
}

/*# ###
G�n�rateur de graphes - v1.4 - � Cyril Gavoille - Octobre 2007

USAGE

       gengraph [-option] graph_name parameter_list


DESCRIPTION

       Permet de g�n�rer un graphe non orient� sous la forme, par
       d�faut, d'une liste d'ar�tes. En param�tre figure le type de
       graphe ainsi que ses param�tres �ventuels (par exemple le
       nombre de sommets, etc.).

   LE FORMAT

       Le format par d�faut est une liste d'ar�tes au format
       texte. Les sommets sont num�rot�s cons�cutivement de 0 � n-1 o�
       n est le nombre de sommets pr�sents dans le graphe. Une ar�te
       entre i et j est repr�sent�e par i-j. Les sommets isol�s sont
       simplement repr�sent�s par le num�ro du sommet suivit d'un
       espace ou d'un retour de ligne. Pour r�sumer, le nombre de
       sommets du graphe est l'entier le plus grand + 1, et s'il y a
       i-j, alors il existe une ar�te entre les sommets i et j.

       Pour une repr�sentation plus compact, les ar�tes cons�cutives
       d'un chemin du graphe peuvent �tre regroup�es en blocs. Par
       exemple, les deux ar�tes 3-5 et 5-8 peuvent �tre regroup�es en
       3-5-8. Les sommets isol�s et les ar�tes (ou les blocs d'ar�tes)
       sont s�par�s par des espaces ou des sauts de ligne. Une boucle
       sur un sommet i est cod�e par i-i. Les ar�tes multiples sont
       cod�es par la r�p�tition d'une m�me ar�te, comme par exemple
       i-j i-j, ou encore i-j-i.

       Quelques exemples:

       Ex1: 0 1-2-3-1

       repr�sente un graphe � 4 sommets, compos� d'un sommet isol� (0)
       et d'un cycle � trois sommets (1,2,3). En voici une
       repr�sentation graphique possible:

              0   1
                 / \
                3---2

       Ex2: 4-2-1-0-3-2

       repr�sente un graphe � 5 sommets compos� d'un cycle de longueur
       4 et d'un sommet de degr� 1 attach� � 2. En voici une
       repr�sentation graphique possible:

                  1
                 / \
              4-2   0
                 \ /
                  3

   COMMENT LE GENERATEUR FONCTIONNE ?

       Pour chaque graphe une fonction adj(i,j) est d�finie. Elle
       fournit l'adjacence entre les sommets de num�ro i et j. Le
       graphe est affich� en g�n�rant toutes les paires {i,j}
       possibles, et en appelant adj(i,j). Les graphes sont ainsi
       g�n�r�s de mani�re implicite, les ar�tes du graphe ne sont pas
       stock�es en m�moire, mais affich�es directement � la
       vol�e. Ceci permet de g�n�rer des graphes de tr�s grande taille
       sans n�cessiter de m�moire centrale. Pour certains graphes,
       comme les arbres ou les graphes d'intersections al�atoires, une
       structure de donn�es de taille O(n) (n nombre de sommets du
       graphe) est cependant utilis�e. Pour les formats d'affichage
       matrix, smatrix et list, une matrice de taille n x n est
       utilis� en interne.


OPTIONS


-help

       Affiche ce texte, c'est-�-dire l'aide en ligne, qui est
       contenue dans le fichier source (.c) du g�n�rateur. Pour cela,
       le fichier source doit �tre dans le m�me r�pertoire que la
       commande.


-permute

       Permet de permuter al�atoirement le nom des sommets qui restent
       dans l'intervalle [0,n[ o� n est le nombre de sommets du graphe
       g�n�r�. Voir aussi l'option -shift.


-dele p
      
       Permet de supprimer chaque ar�te du graphe g�n�r�e avec
       probabilit� p.


-delv p

       Similaire � -dele p, mais concerne les sommets. Le sommets et
       ses ar�tes incidentes sont alors supprim�es. Si k sommets sont
       supprim�s, alors le nom des sommets restant sont dans
       l'intervalle [0,n-k[ o� n est le nombre de sommets initial du
       graphe. Les noms des sommets sont donc �ventuellement
       renum�rot�s. Voir aussi les options -permute et -shift.


-not

       Inverse la fonction d'adjacence, et donc affiche le compl�ment du
       graphe.


-seed s

       Permet d'initialiser le g�n�rateur al�atoire avec la graine s,
       permettant de g�n�rer plusieurs fois la m�me suite
       al�atoire. Par d�faut, la graine est initialis�e avec le num�ro
       de processus de la commande, donc g�n�re par d�faut des suites
       diff�rentes � chaque lancement.


-width n

       Limite � n le nombre d'ar�tes et de sommets isol�s affich�s par
       ligne. Cette option n'a pas de signification particuli�re en
       dehors du format standard. Par exemple, -width 1 affiche une
       arr�te (ou un sommet isol�) par ligne. L'option -width 0
       affiche tout sur une seule ligne.


-shift s

       Permet de renum�roter les sommets � partir de s, qui doit �tre
       un entier >= 0. La valeur par d�faut est -shift 0. L'int�r�t de
       cette option est de pouvoir r�aliser des unions de graphes
       simplement en renum�rotant les sommets et en concat�nant les
       fichiers (valable aussi pour le format list).


-format type

       Valeurs possibles pour type:

       - standard : format standard (liste d'ar�tes), c'est le plus compacte.
       - matrix : matrice d'adjacence
       - smatrix : affiche la matrix inf�rieure droite
       - list : affiche la liste d'adjacence


GRAPHES

       Deux types de graphes sont possibles : les graphes de base et les
       graphes compos�s. Ces derniers peuvent �tre obtenus en modifiant, �
       l'aide des options, un graphe de base.


  GRAPHES DE BASE :

    grid d n_1 ... n_d

       Grille � d dimensions de taille n_1 x ... x n_d. Si la
       dimension n_i est n�gative, alors cette dimension est cyclique.
       Par exemple, "grid 1 -10" donnera un cycle � 10 sommets.

    ring n k c_1 ... c_k

       Anneaux de cordes � n sommets avec k cordes de longueur
       c_1,...,c_k.

    tree n

       Arbre plan enracin� al�atoire � n sommets.

    outerplanar n

       Graphe planaire ext�rieur connexe � n sommets (plan et
       enracin�). Ils sont en bijection avec les arbres plans
       enracin�s dont tous les sommets, sauf ceux de la derni�re
       branche, sont bicolor�s.

    kneser n k r

       Graphe de Kneser g�n�ralis�, le graphe de Kneser K(n,k)
       classique est obtenu avec r=0. Les sommets sont tous les
       sous-ensembles � k �l�ments de [0,n[, et i et j sont adjacents
       ssi leurs ensembles correspondant ont au plus r �l�ments en
       commun. Le nombre chromatique de K(n,k), �tablit par Lovasz,
       vaut n-2k+2 pour tout n>=2k-1>0.

    interval n

       Graphe d'intersection de n intervalles d'entiers al�atoires
       uniforme de [0,2n[.

    permutation n

       Graphe de permutation sur une permutation al�atoire uniforme de
       [0,n[.

    prime n

       Graphe � n sommets tel que i-j ssi i>1 et j divisible par i.

    sat n m k

       Graphe issu de la r�duction du probl�me k-SAT � Vertex
       Cover. Soit F une formule de k-SAT avec n variables x_i et m
       clauses de k termes. Le graphe g�n�r� par "sat n m k" poss�de
       un Vertex Cover de taille n+(k-1)m si et seulement si F est
       satisfiable. Ce graphe est compos� d'une union de n ar�tes et
       de m cliques de k sommets, plus des ar�tes connectant certains
       sommets des cliques aux n ar�tes. Les n ar�tes repr�sentent les
       n variables, une extr�mit� pour x_i, l'autre pour non(x_i). Ces
       sommets ont des num�ros dans [0,2n[, x_i correspond toujours �
       un num�ro pair. Les sommets des cliques ont des num�ros
       cons�cutifs >= 2n. Le j-�me sommet de la i-�me clique est
       connect� � l'une des extr�mit�s de l'ar�te k ssi la j-�me
       variable de la i-�me clause est x_k (ou non(x_i)). Les
       connexions sont al�atoires uniformes.

    kout n k

       Graphe � n sommets cr�e par le processus al�atoire suivant: les
       sommets sont ajout�s dans l'ordre croissant de leur num�ro,
       i=0,1,...n-1. Le sommet i est connect� � d voisins qui sont
       pris al�atoirement uniforme dans les sommets dont le num�ro est
       < i. Le degr� d est d�termin� al�atoirement uniforme entre 0 et
       min{j,k}. Ces graphes ont au plus kn ar�tes et sont k+1
       coloriables.


  GRAPHES COMPOSES :

    mesh p q (grid 2 p q)

       Grille 2D de p x q sommets.

    hypercube d (grid d 2 ... 2)

       Hypercube de dimension d.

    path n (grid 1 n)

       Chemin � n sommets.

    cycle n (ring n 1 1)

       Cycle � n sommets.

    torus p q (grid 2 -p -q)

       Tore � p x q sommets.

    stable n (ring n 0)

       Stable � n sommets.

    clique n (-not ring n 0)

       Graphe complet � n sommets.

    random n p (-not ring n 0 -dele 1-p)

       Graphe al�atoire � n sommets et de probabilit� d'avoir une
       ar�te p.

    petersen (kneser 5 2 0)

       Graphe de Kneser particulier bien connu � 10 sommets et
       3-r�gulier.


A FAIRE :

       sparse k n: union de k arbres plans � n sommets
       geometric n r: intersection graph of radius-r balls in [0,1]^2
       -redirect p: avec proba p redirige l'ar�te uniform�ment
       Graphes orient�s: i->j


HISTORIQUE:

       v1.2 octobre 2007:
            - premi�re version

       v1.3 octobre 2008:
            - nouvelles options: -shift, -width
            - correction d'un bug pour les graphes de permutation
	    - acc�l�ration du test d'ajacence pour les arbres, de O(n) � O(1)
              gr�ce � la repr�sentation implicite
	    - nouveau graphes: outerplanar n ; sat n m k

       v1.4 novembre 2008:
            - format de sortie: matrix, smatrix et list
            - nouveau graphe: kout n k
            - correction d'un bug dans l'option -width.

### #*/
