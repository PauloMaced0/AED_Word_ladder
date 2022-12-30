//
// AED, November 2022 (Tomás Oliveira e Silva)
//
// Second practical assignement (word ladder)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// static configuration
//

// SEE MALLOCS AND FREES
#define malloc(s) ({void *m = malloc(s); printf("%016lx M %3d %10ld\n",(long)m,__LINE__,(long)(s)); m;})
#define free(s) ({free(s); printf("%016lx F %3d\n",(long)s,__LINE__);})
#define _max_word_size_  32


//
// data structures 
//

typedef struct adjacency_node_s  adjacency_node_t;
typedef struct hash_table_node_s hash_table_node_t;
typedef struct hash_table_s      hash_table_t;

struct adjacency_node_s
{
  adjacency_node_t *next;            // link to th enext adjacency list node
  hash_table_node_t *vertex;         // the other vertex
};

struct hash_table_node_s
{
  // the hash table data
  char word[_max_word_size_];        // the word
  hash_table_node_t *next;           // next hash table linked list node
  // the vertex data
  adjacency_node_t *head;            // head of the linked list of adjancency edges
  int visited;                       // visited status (while not in use, keep it at 0)
  hash_table_node_t *previous;       // breadth-first search parent
  // the union find data
  hash_table_node_t *representative; // the representative of the connected component this vertex belongs to
  int number_of_vertices;            // number of vertices of the conected component (only correct for the representative of each connected component)
  int number_of_edges;               // number of edges of the conected component (only correct for the representative of each connected component)
};

struct hash_table_s
{
  unsigned int hash_table_size;      // the size of the hash table array
  unsigned int number_of_entries;    // the number of entries in the hash table
  unsigned int number_of_edges;      // number of edges (for information purposes only)
  hash_table_node_t **heads;         // the heads of the linked lists
};


//
// allocation and deallocation of linked list nodes 
//

static adjacency_node_t *allocate_adjacency_node(void)
{
  adjacency_node_t *node;

  node = (adjacency_node_t *)malloc(sizeof(adjacency_node_t));
  if(node == NULL)
  {
    fprintf(stderr,"allocate_adjacency_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_adjacency_node(adjacency_node_t *node)
{
  free(node);
}

static hash_table_node_t *allocate_hash_table_node(void)
{
  hash_table_node_t *node;

  node = (hash_table_node_t *)malloc(sizeof(hash_table_node_t));
  if(node == NULL)
  {
    fprintf(stderr,"allocate_hash_table_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_hash_table_node(hash_table_node_t *node)
{
  free(node);
}


//
// hash table stuff
//

unsigned int crc32(const char *str)
{
  static unsigned int table[256];
  unsigned int crc;

  if(table[1] == 0u) // do we need to initialize the table[] array?
  {
    unsigned int i,j;

    for(i = 0u;i < 256u;i++)
      for(table[i] = i,j = 0u;j < 8u;j++)
        if(table[i] & 1u)
          table[i] = (table[i] >> 1) ^ 0xAED00022u; // "magic" constant
        else
          table[i] >>= 1;
  }
  crc = 0xAED02022u; // initial value (chosen arbitrarily)
  while(*str != '\0')
    crc = (crc >> 8) ^ table[crc & 0xFFu] ^ ((unsigned int)*str++ << 24);
  return crc;
}

static hash_table_t *hash_table_create(void)
{
  hash_table_t *hash_table;
  unsigned int i;

  hash_table = (hash_table_t *)malloc(sizeof(hash_table_t));
  if(hash_table == NULL)
  {
    fprintf(stderr,"create_hash_table: out of memory\n");
    exit(1);
  }
  hash_table->hash_table_size = 100; //initial size of the hash table
  hash_table->number_of_entries = 0;
  hash_table->number_of_edges = 0;
  hash_table->heads = (hash_table_node_t **)malloc(sizeof(hash_table_node_t) * hash_table->hash_table_size);
  if(hash_table->heads == NULL){
    fprintf(stderr,"hash_table_table: out of memory\n");
    exit(1);
  }
  for(i = 0; i < hash_table->hash_table_size; i++){
    hash_table->heads[i] = NULL;
  }
  printf("hash table created (size: %d)\n",hash_table->hash_table_size);
  return hash_table;
}

static void hash_table_free(hash_table_t *hash_table)
{
  if(hash_table == NULL)
    return;
  for (unsigned int i = 0; i < hash_table->hash_table_size; i++)
  {
    hash_table_node_t *entry = hash_table->heads[i];
    while(entry != NULL){
      hash_table_node_t *last_entry = entry;
      adjacency_node_t *adjacency_node = entry->head;
      while(adjacency_node != NULL){
        adjacency_node_t *last_adjacency = adjacency_node;
        adjacency_node = last_adjacency->next;
        free_adjacency_node(last_adjacency);
      }
      entry = last_entry->next;
      free_hash_table_node(last_entry); 
    }
  }
  free(hash_table->heads);
  free(hash_table);
}

static void hash_table_grow(hash_table_t *hash_table)
{
  unsigned int new_size = hash_table->hash_table_size * 2;
  // Create a new hash table 
  hash_table_node_t **new_heads = (hash_table_node_t **)malloc(sizeof(hash_table_node_t) * new_size);
  if(new_heads == NULL) exit(1);

  // Initialize new hash table
  for (unsigned int i = 0; i < new_size; i++)
    new_heads[i] = NULL;

  for(unsigned int i = 0; i < hash_table->hash_table_size; i++){
    hash_table_node_t *entry = hash_table->heads[i];
    while(entry != NULL){
      //Insert node into the new hash table
      hash_table_node_t *last_entry = entry;
      unsigned int new_index = crc32(entry->word) % new_size;
      if(new_heads[new_index] == NULL){
        new_heads[new_index] = entry;
      } else {
        hash_table_node_t *current = new_heads[new_index];
        while(current->next != NULL)
          current = current->next;
        current->next = entry;
      }
      entry = entry->next;
      last_entry->next = NULL; 
    }
  }
  //Free the old hash table and assign the new one
  free(hash_table->heads);
  hash_table->heads = new_heads;
  hash_table->hash_table_size = new_size;
  printf("hash table resized (new size: %d, entries: %d)\n", new_size, hash_table->number_of_entries);
}

static hash_table_node_t *find_word(hash_table_t *hash_table,const char *word,int insert_if_not_found)
{
  hash_table_node_t *node;
  unsigned int i;

  i = crc32(word) % hash_table->hash_table_size;
  //Find the node 
  node = hash_table->heads[i];
  while (node != NULL)
  {
    if(strcmp(node->word,word) == 0)
      break;
    node = node->next;
  }
  //If the node wasn't found and we must insert it
  if(node == NULL && insert_if_not_found)
  {
    if((unsigned int)(hash_table->hash_table_size * 0.6) <= hash_table->number_of_entries)
      hash_table_grow(hash_table);
    node = allocate_hash_table_node();
    if(node == NULL) exit(1);
    strcpy(node->word, word);
    // Node values
    node->next = NULL;
    node->previous = NULL;
    node->representative = node;
    node->number_of_edges = 0;
    node->number_of_vertices = 1;
    node->visited = 0;
    node->head = NULL;
    //Insert the node into the linked list at the hash index
    if(hash_table->heads[i] == NULL){
      hash_table->heads[i] = node;
    } else {
      hash_table_node_t *current = hash_table->heads[i];
      while (current->next != NULL)
      {
        current = current->next;
      }
      current->next = node;
    }
    hash_table->number_of_entries += 1;
  }
  
  return node;
}

//
// add edges to the word ladder graph 
//

static hash_table_node_t *find_representative(hash_table_node_t *node)
{
  hash_table_node_t *representative,*next_node,*n;
  for(representative = node;representative->representative != representative; representative = representative->representative)
  ;
  for(n = node; n != representative; n = next_node){
    next_node = n->representative;
    n->representative = representative;
  }
  return representative;
}

static void add_edge(hash_table_t *hash_table,hash_table_node_t *from,const char *word)
{
  hash_table_node_t *to,*from_rep,*to_rep;
  adjacency_node_t *link;
  to = find_word(hash_table,word,0);
  if(to == NULL)
    return;
  // find the representative of the two nodes
  from_rep = find_representative(from);
  to_rep = find_representative(to);

  // add the edge between the two nodes 
  for(link = from->head; link != NULL && link->vertex != to; link = link->next)
    ;
  if(link != NULL)   // If adjacency is already registered, return
    return;
  link = allocate_adjacency_node();
  link->next = from->head;
  from->head = link;
  link->vertex = to;
  adjacency_node_t *link2; // Add a link in 'node to' as well
  for(link2 = to->head; link2 != NULL && link2->vertex != from; link2 = link2->next)
    ;
  link2 = allocate_adjacency_node();
  link2->next = to->head;
  to->head = link2;
  link2->vertex = from;
    

  // if the representatives are not the same, make one representative of the other
  if(from_rep != to_rep)
  {
    if(from_rep->number_of_vertices >= to_rep->number_of_vertices){
      to_rep->number_of_vertices += from_rep->number_of_vertices;
      to_rep->number_of_edges += from_rep->number_of_edges + 1;  
      from_rep->representative = to_rep;
    } else {
      from_rep->number_of_vertices += to_rep->number_of_vertices;
      from_rep->number_of_edges += to_rep->number_of_edges + 1;
      to_rep->representative = from_rep;
    }
  }
  else
  {
    from_rep->number_of_edges += 1;
  }
  hash_table->number_of_edges += 1;
}

//
// generates a list of similar words and calls the function add_edge for each one
//
// man utf8 for details on the uft8 encoding
//

static void break_utf8_string(const char *word,int *individual_characters)
{
  int byte0,byte1;

  while(*word != '\0')
  {
    byte0 = (int)(*(word++)) & 0xFF;
    if(byte0 < 0x80)
      *(individual_characters++) = byte0; // plain ASCII character
    else
    {
      byte1 = (int)(*(word++)) & 0xFF;
      if((byte0 & 0b11100000) != 0b11000000 || (byte1 & 0b11000000) != 0b10000000)
      {
        fprintf(stderr,"break_utf8_string: unexpected UFT-8 character\n");
        exit(1);
      }
      *(individual_characters++) = ((byte0 & 0b00011111) << 6) | (byte1 & 0b00111111); // utf8 -> unicode
    }
  }
  *individual_characters = 0; // mark the end!
}

static void make_utf8_string(const int *individual_characters,char word[_max_word_size_])
{
  int code;

  while(*individual_characters != 0)
  {
    code = *(individual_characters++);
    if(code < 0x80)
      *(word++) = (char)code;
    else if(code < (1 << 11))
    { // unicode -> utf8
      *(word++) = 0b11000000 | (code >> 6);
      *(word++) = 0b10000000 | (code & 0b00111111);
    }
    else
    {
      fprintf(stderr,"make_utf8_string: unexpected UFT-8 character\n");
      exit(1);
    }
  }
  *word = '\0';  // mark the end
}

static void similar_words(hash_table_t *hash_table,hash_table_node_t *from)
{
  static const int valid_characters[] =
  { // unicode!
    0x2D,                                                                       // -
    0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,           // A B C D E F G H I J K L M
    0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,           // N O P Q R S T U V W X Y Z
    0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,           // a b c d e f g h i j k l m
    0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,           // n o p q r s t u v w x y z
    0xC1,0xC2,0xC9,0xCD,0xD3,0xDA,                                              // Á Â É Í Ó Ú
    0xE0,0xE1,0xE2,0xE3,0xE7,0xE8,0xE9,0xEA,0xED,0xEE,0xF3,0xF4,0xF5,0xFA,0xFC, // à á â ã ç è é ê í î ó ô õ ú ü
    0
  };
  int i,j,k,individual_characters[_max_word_size_];
  char new_word[2 * _max_word_size_];

  break_utf8_string(from->word,individual_characters);
  for(i = 0;individual_characters[i] != 0;i++)
  {
    k = individual_characters[i];
    for(j = 0;valid_characters[j] != 0;j++)
    {
      individual_characters[i] = valid_characters[j];
      make_utf8_string(individual_characters,new_word);
      // avoid duplicate cases
      if(strcmp(new_word,from->word) > 0)
        add_edge(hash_table,from,new_word);
    }
    individual_characters[i] = k;
  }
}

//
// returns the number of vertices visited; if the last one is goal, following the previous links gives the shortest path between goal and origin
//
static int breadth_first_search(hash_table_node_t **list_of_vertices,hash_table_node_t *origin,hash_table_node_t *goal){
  int r = 0;
  int w = 1;
  list_of_vertices[0] = origin;
  origin->visited = 1;
  int distance = 0;
  hash_table_node_t *n;
  adjacency_node_t *nn;
  hash_table_node_t *last_node = NULL;
  while (r < w)
  {
    n = list_of_vertices[r++];
    for (nn = n->head; nn != NULL; nn = nn->next)
    {
      if (nn->vertex->visited == 0)
      {
        list_of_vertices[w++] = nn->vertex;
        nn->vertex->visited = 1;
        nn->vertex->previous = n;
        if (nn->vertex == goal)
        {
          list_of_vertices[w] = NULL;
          for (n = list_of_vertices[0], r = 1; r != w + 1; n = list_of_vertices[r++])
          {
            n->visited = 0;
          }
          return 0;
        }
        last_node = nn->vertex;
      }
    }
  }
  list_of_vertices[w] = NULL;
  for (n = list_of_vertices[0], r = 1; r != w + 1; n = list_of_vertices[r++])
  {
    n->visited = 0;
  }
  if (goal == NULL)
  {
    list_of_vertices[0] = last_node;
    if (last_node != NULL){
      for(; last_node != origin; last_node = last_node->previous)
        distance++;
    }
    return distance;
  }
  return -1;
}

//
// list all vertices belonging to a connected component 
//
static void list_connected_component(hash_table_t *hash_table,const char *word)
{
  // Get the vertex corresponding to the given word
  hash_table_node_t *vertex = find_word(hash_table,word,0);
  if(vertex == NULL)
    return;
  // Get the representative of the connected component
  hash_table_node_t *representative = find_representative(vertex);
  if(representative == NULL)
    return;

  // Iterate through the hash table 
  for(unsigned int i = 0; i<hash_table->hash_table_size; i++){
    hash_table_node_t *current_node = hash_table->heads[i];
    while(current_node != NULL){
      // Check if the node is part of the connected component
      if(find_representative(current_node) == representative){
        printf("%s\n",current_node->word);
      }
      current_node = current_node->next;
    }
  }
  printf("Component vertices: %d\n", vertex->representative->number_of_vertices);
  printf("Component edges: %d\n", vertex->representative->number_of_edges);
}
//
// find the shortest path from a given word to another given word 
//
static void path_finder(hash_table_t *hash_table,const char *from_word,const char *to_word)
{
  hash_table_node_t *nn;
  hash_table_node_t *from = find_word(hash_table,from_word,0);
  hash_table_node_t *to = find_word(hash_table,to_word,0);
  int component_vertices = find_representative(from)->number_of_vertices;
  hash_table_node_t **list_of_vertices = (hash_table_node_t **)malloc(sizeof(hash_table_node_t) * component_vertices);
  if(from == NULL || to == NULL) return;
  if (breadth_first_search(list_of_vertices,from,to) == -1){
    printf("There's no path!\n");
  } else {
    int i = 0;
    hash_table_node_t *last_node = NULL;
    from->previous = NULL;
    for(hash_table_node_t *n = to; n != NULL; n = nn){
      nn = n->previous;
      n->previous = last_node;
      last_node = n;
    }
    for(hash_table_node_t *n = from; n != to; n = n->previous)
      printf("[%2d] %s\n",i++,n->word);
    printf("[%2d] %s\n", i, to->word);
  }
  free(list_of_vertices);
}

//
// compute the diameter of a connected component 
//

static int connected_component_diameter(hash_table_t *hash_table, hash_table_node_t *node, int print_this_diameter)
{
  if(node == NULL) return -1;
  int component_diameter = 0;
  int diameter = 0;
  int i = 1;
  int component_vertices = find_representative(node)->number_of_vertices;
  hash_table_node_t **list_of_component_vertices = (hash_table_node_t **)malloc(sizeof(hash_table_node_t) * component_vertices);
  breadth_first_search(list_of_component_vertices,node,NULL); // The first breadth first search that will save all vertices on list_of_vertices
  for(; list_of_component_vertices[i] != NULL && i<component_vertices; i++){
    hash_table_node_t **list_of_vertices = (hash_table_node_t **)malloc(sizeof(hash_table_node_t) * component_vertices);
    node = list_of_component_vertices[i];
    diameter = breadth_first_search(list_of_vertices, node, NULL);
    if(diameter == print_this_diameter){
      path_finder(hash_table, node->word, list_of_vertices[0]->word);
      print_this_diameter = -1;
    }
    if(diameter > component_diameter && diameter >= 0){ // Store the maximum of the smallest diameters
      component_diameter = diameter;
    }
    free(list_of_vertices);
  }
  free(list_of_component_vertices);

  return component_diameter;
}

// some graph information 
//

static void graph_info(hash_table_t *hash_table)
{
  int j;
  int num_of_components = 0;
  int diameter;
  int largest_diameter_count = 0, largest_diameter = 0;
  hash_table_node_t *largest_diameter_node = NULL;
  hash_table_node_t **list_of_components = (hash_table_node_t **)malloc(sizeof(hash_table_node_t) * (hash_table->hash_table_size)/2);
  hash_table_node_t *entry = NULL;
  for(unsigned int i = 0; i < hash_table->hash_table_size; i++){
    entry = hash_table->heads[i];
    while (entry != NULL)
    {
      hash_table_node_t *representative = find_representative(entry);
      for(j = 0; j < num_of_components && representative != list_of_components[j]; j++)
        ;
      if(j == num_of_components)
        list_of_components[num_of_components++] = representative;
      entry = entry->next;
    }
  }
  int store_diameters[1000] = {0};
  j= 1;
  for(hash_table_node_t *node = list_of_components[0] ; j < num_of_components; node = list_of_components[j++]){
    diameter = connected_component_diameter(hash_table, node, -1);
    if(diameter >= 0) store_diameters[diameter]++;
    if (diameter == largest_diameter) largest_diameter_count++;
    if(diameter > largest_diameter){
      largest_diameter_count = 1;
      largest_diameter = diameter;
      largest_diameter_node = node;
    }
  }
  free(list_of_components);
  printf("%d vertices\n",hash_table->number_of_entries);
  printf("%d edges\n", hash_table->number_of_edges);
  printf("%d connected components\n",num_of_components);
  printf("Number of connected components with a diameter of:\n");
  for(j = 0; j<=largest_diameter; j++){
    if(store_diameters[j] != 0) 
      printf(" %d: %d\n", j, store_diameters[j]);
  }
  printf("largest word ladder:\n");
  connected_component_diameter(hash_table, largest_diameter_node, largest_diameter);
}

//
// main
//

int main(int argc,char **argv)
{
  char word[100],from[100],to[100];
  hash_table_t *hash_table;
  hash_table_node_t *node;
  unsigned int i;
  int command;
  FILE *fp;

  // initialize hash table
  hash_table = hash_table_create(); 
  // read words
  fp = fopen((argc < 2) ? "wordlist-big-latest.txt" : argv[1],"rb");
  if(fp == NULL)
  {
    fprintf(stderr,"main: unable to open the words file\n");
    exit(1);
  }
  while(fscanf(fp,"%99s",word) == 1)
    (void)find_word(hash_table,word,1);
  while(fscanf(fp,"%99s",word) == 1)
    if(find_word(hash_table,word,0) == NULL || find_word(hash_table,word,0)->word != word) exit(1);
  fclose(fp);
  for(i = 0u;i < hash_table->hash_table_size;i++)
    for(node = hash_table->heads[i];node != NULL;node = node->next)
      similar_words(hash_table,node);
  graph_info(hash_table);
  for(;;)
  {
    fprintf(stderr,"Your wish is my command:\n");
    fprintf(stderr,"  1 WORD       (list the connected component WORD belongs to)\n");
    fprintf(stderr,"  2 FROM TO    (list the shortest path from FROM to TO)\n");
    fprintf(stderr,"  3            (terminate)\n");
    fprintf(stderr,"> ");
    if(scanf("%99s",word) != 1)
      break;
    command = atoi(word);
    if(command == 1)
    {
      if(scanf("%99s",word) != 1)
        break;
      list_connected_component(hash_table,word);
    }
    else if(command == 2)
    {
      if(scanf("%99s",from) != 1)
        break;
      if(scanf("%99s",to) != 1)
        break;
      path_finder(hash_table,from,to);
    }
    else if(command == 3)
      break;
  }
  // clean up
  hash_table_free(hash_table);
  return 0;
}
