#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "graph_input.h"

#define INFINITY 0xFFFF

int N, M, directed, weighted;
int * edge_matrix;
char ** node_list;
int * previous;
int * distances;

void Dijkstra(int sourcenode);
bool validNeighbour(int i, int j);
int * newPriorityQueueFromEdgeMatrix(int sourcenode);
void changeDistance(int key, int newDistance);
int extractMinDistance(void);
bool allNodesVisited(void);
void printOptimalPaths(int curr_node);

int main (void)
{
  printf("\nWelcome to dijkstra.c!\n");

  getParameters(&N, &M, &directed, &weighted);
  node_list = getNodeList(N);
  assert(node_list);
  bool negativeEdgesOkay = false;
  edge_matrix = getGraph(N, M, directed, weighted, node_list, negativeEdgesOkay);
  assert(edge_matrix);
  int sourcenode = getSourceNode(N, node_list);

  Dijkstra(sourcenode);

  deleteGraph(N, edge_matrix, node_list);

  return 0;
}

void Dijkstra(int sourcenode)
{
  distances = newPriorityQueueFromEdgeMatrix(sourcenode); // Sets source distance to 0 and rest to infinity
  assert(distances);
  previous = malloc(N * sizeof(int));
  assert(previous);
  for (int i = 0; i < N; i++)
  {
    previous[i] = (i == sourcenode) ? sourcenode : -1; // Previous node in optimal path undefined
  }

  while(!allNodesVisited())
  {
    int nextNode = extractMinDistance();
    for (int j = 0; j < N; j++)
    {
      if (validNeighbour(nextNode, j))
      {
        int newPathLength = distances[nextNode] + edge_matrix[nextNode*N + j];
        if (newPathLength < distances[j])
        {
          changeDistance(j, newPathLength);
          previous[j] = nextNode;
        }
      }
    }
    changeDistance(nextNode, -1); // Mark node as reached
  }

  printf("Printing optimal paths:\n");
  for(int i = 0; i < N; i++)
  {
    printOptimalPaths(i);
    printf("\n");
  }

  free(distances);
  free(previous);
}

bool validNeighbour(int i, int j)
{
  bool valid = true;
  if (!edgeExists(i, j, N, edge_matrix)) // no edge between i and j
  {
    valid = false;
  }
  if (distances[j] == -1) // already visited node
  {
    valid = false;
  }
  return valid;
}

int * newPriorityQueueFromEdgeMatrix(int sourcenode)
{
  int * p_queue = malloc(N * sizeof(int));
  assert(p_queue);
  for (int i = 0; i < N; i++)
  {
    p_queue[i] = (i == sourcenode) ? 0 : INFINITY;
  }
  return p_queue;
}

void changeDistance(int key, int newDistance)
{
  assert(key < N);
  assert(newDistance < INFINITY);
  distances[key] = newDistance;
}

int extractMinDistance(void)
{
  int i;
  int min = INFINITY;
  int min_index = -1; // returns -1 if distances is empty
  for (i = 0; i < N; i++)
  {
    if (distances[i] != -1 && distances[i] <= min)
    {
      min = distances[i];
      min_index = i;
    }
  }
  return min_index;
}

bool allNodesVisited(void)
{
  bool empty = true;
  for (int i = 0; i < N; i++)
  {
    if (distances[i] != -1)
    {
      empty = false;
    }
  }
  return empty;
}

void printOptimalPaths(int curr_node)
{
  printf("%s", node_list[curr_node]);
  if (previous[curr_node] != curr_node)
  {
    printf(" <= ");
    printOptimalPaths(previous[curr_node]);
  }
}