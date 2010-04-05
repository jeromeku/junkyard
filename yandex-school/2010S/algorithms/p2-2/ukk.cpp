// http://acm.mipt.ru/twiki/bin/view/Algorithms/UkkonenCPP
/**
 * ukk.cpp
 * My implementation of the Ukkonen algo. Based on
 * Ukkonen's paper on this topic.
 * Daniil Shved, MIPT, 2009.
 */
#include <vector>
#include <algorithm>
#include <string>
#include <limits>
#include <iostream>
#include <cstdio>
#include <cassert>
using namespace std;

enum { ALPHABET_SIZE = 26 };

const int inf = numeric_limits<int>::max();
typedef unsigned char UChar;

// Represents a link in our suffix tree
struct Link {
   int start, end;
   int to;

   // default: invalid link
   Link() {
      to = -1;
   }

   // a link with given parameters
   Link(int _start, int _end, int _to) {
      start = _start;
      end = _end;
      to = _to;
   }
};

// Represents an explicit vertex in our suffix tree
struct Vertex {
   Link links[26];      // state links
   int suffix;          // suffix link

   Vertex() {
      suffix = -1;
   }
};

// The whole suffix tree
vector<Vertex> tree;
int root, dummy;

// The sample it is built for
string sample;

// Gets the character with the given index. Understands negative indices
UChar t(int i) {
   return (i<0) ? (-i-1) : sample[i];
}

// Creates a new vertex in the suffix tree
int newVertex()
{
   int i = tree.size();
   tree.push_back(Vertex());
   return i;
}

// Creates a link in the suffix tree
// to, from - two vertices
// [start, end) - the word on the edge
void link(int from, int start, int end, int to)
{
   tree[from].links[t(start)] = Link(start, end, to);
}

// The f function (goes along the suffix link)
int &f(int v)
{
   return tree[v].suffix;
}

// Initializes the suffix tree
// creates two vertices: root and dummy (root's parent)
void initTree()
{
   tree.clear();
   dummy = newVertex();
   root = newVertex();

   f(root) = dummy;
   for(int i=0; i<ALPHABET_SIZE; i++)
      link(dummy, -i-1, -i, root);
}

// Canonizes the reference pair (v, (start, end)) of a state (probably implicit)
pair<int, int> canonize(int v, int start, int end)
{
   if(end <= start) {
      return make_pair(v, start);
   } else {
      Link cur = tree[v].links[t(start)];
      while(end - start >= cur.end - cur.start) {   
         start += cur.end - cur.start;
         v = cur.to;
         if(end > start)
            cur = tree[v].links[t(start)];
      }
      return make_pair(v, start);
   }
}

// Checks if there is a t-transition from the (probably implicit)
// state (v, (start, end))
pair<bool, int> testAndSplit(int v, int start, int end, UChar c)
{
   if(end <= start) {
      return make_pair(tree[v].links[c].to != -1, v);
   } else {
      Link cur = tree[v].links[t(start)];
      if(c == t(cur.start + end - start))
         return make_pair(true, v);

      int middle = newVertex();
      link(v, cur.start, cur.start + end - start, middle);
      link(middle, cur.start + end - start, cur.end, cur.to);
      return make_pair(false, middle);
   }
}

// Creates new branches
// (v, (start, end)) - the active point (its canonical reference pair)
//
// We want to add a t(end)-transition to this point, and to f(of it), f(f(of
// it)) and so on up to the end point
//
// NOTE: end must be a correct index in the sample string
pair<int, int> update(int v, int start, int end) {
   Link cur = tree[v].links[t(start)];
   pair<bool, int> splitRes;
   int oldR = root;

   splitRes = testAndSplit(v, start, end, t(end));
   while(!splitRes.first) {
      // Add a new branch
      link(splitRes.second, end, inf, newVertex());

      // Create a suffix link from the prev. branching vertex
      if(oldR != root)
         f(oldR) = splitRes.second;
      oldR = splitRes.second;

      // Go to the next vertex (in the final set of STrie(T_end))
      pair<int, int> newPoint = canonize(f(v), start, end);
      v = newPoint.first;
      start = newPoint.second;
      splitRes = testAndSplit(v, start, end, t(end));
   }
   if(oldR != root)
      f(oldR) = splitRes.second;
   return make_pair(v, start);
}

// Builds the whole suffix tree for the string sample
void ukkonen()
{
   // Initialize the tree
   initTree();

   // Add characters one by one
   pair<int, int> activePoint = make_pair(root, 0);
   for(int i=0; i<sample.length(); i++) {
      activePoint = update(activePoint.first, activePoint.second, i);
      activePoint = canonize(activePoint.first, activePoint.second, i+1);
   }
}

int ComputeNumberOfDistinctSubstrings() {
    for (int i = 0; i < sample.size(); i++)
        sample[i] -= 'a';

    ukkonen();

    int sum_of_edge_lengths = 0;
    for (size_t vertex_id = 0; vertex_id < tree.size(); vertex_id++) {
        if (vertex_id == dummy)
            continue;

        const Vertex &vertex = tree[vertex_id];
        for (int link_id = 0; link_id < ALPHABET_SIZE; link_id++) {
            const Link &link = vertex.links[link_id];
            if (link.to == -1)  // denotes a non-existing link
                continue;

            assert(0 <= link.start && link.start < link.end && (link.end <= sample.size() || link.end == inf));
            if (link.end == inf)
                sum_of_edge_lengths += sample.size() - link.start;
            else
                sum_of_edge_lengths += link.end - link.start;
        }
    }

    return sum_of_edge_lengths;
}

int main()
{
    cin >> sample;
    printf("%d\n", ComputeNumberOfDistinctSubstrings());
    return 0;
}
