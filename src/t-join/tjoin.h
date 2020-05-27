#ifndef __TJOIN_H__
#define __TJOIN_H__

#include "../../include/pcfl.h"

#include <vector>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <set>
#include <cassert>
#include <iostream>
#include <memory>

namespace tjoin {

	class AuxGraph {
		// n: # of vertices, m: # of edges
	private:
		class Edge{
		private:
			int ver, J, u, v;
			double cost;
		public:
			Edge(int u, int v,double cost);
			int getAdj(int u);
			
			friend class AuxGraph;
			friend void reassign(AuxGraph &G, int V, int n, int m, double *open, double *assign,
					int *sub_tar, int *refinv);
		};

		int n,m;
		std::vector<Edge> edges;
		std::vector<std::vector<int>> adj;
		
	public:
		AuxGraph(int n);
		
		void addEdge(int u,int v,double d);
		void shortestPath(int src, double *dist, int *prev, bool trace);
		void label_edge(int src, int dst, int *prev);
		
		std::vector<std::pair<int,int>> getJ();

		friend void reassign(AuxGraph &G, int V, int n, int m, double *open, double *assign,
				int *sub_tar, int*refinv);
	};

	void solve_join(AuxGraph &G, int n, std::vector<int> &T);
	void sparsify(AuxGraph &G, AuxGraph &ret, int n, double *open, int* subtar, int m);
	void reassign(AuxGraph &G, int V, int n, int m, double *open, double *assign,
			int *sub_tar, int *refinv);

	void approximate_unconstraint(PCFLProbData* data, double *open, double *assign);

	bool check_validity(PCFLProbData *data, int n,int m,double *open, double *assign);
}

#endif
