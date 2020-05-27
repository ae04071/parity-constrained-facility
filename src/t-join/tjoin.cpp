#include "tjoin.h"

#include "../../include/blossom5.h"

namespace tjoin {
	AuxGraph::Edge::Edge(int u,int v,double cost)
		:ver(u^v), J(0), u(u), v(v), cost(cost) {}
	int AuxGraph::Edge::getAdj(int u) {
		return ver ^ u;
	}

	AuxGraph::AuxGraph(int n): n(n), m(0) {
		adj.resize(n);
	}
	void AuxGraph::addEdge(int u,int v,double w) {
		adj[u].push_back(m);
		adj[v].push_back(m);
		edges.push_back(AuxGraph::Edge(u, v, w));
		m++;
	}

	void AuxGraph::shortestPath(int src, double *dist, int *prev, bool trace = false) {
		std::priority_queue<std::pair<double, int>> que;
		std::vector<bool> vis(n, false);

		std::fill(dist, dist+n, 1e100);
		dist[src] = 0;
		que.push({0, src});
		while(!que.empty()) {
			double cost=-que.top().first;
			int cur=que.top().second;
			que.pop();
			
			if(vis[cur]) continue;
			vis[cur] = 1;

			for(auto &i:adj[cur]) {
				int tar = edges[i].getAdj(cur);
				double nxt = cost + edges[i].cost;
				if(!vis[tar] && dist[tar] > nxt + 1e-7) {
					dist[tar] = nxt;
					que.push({-nxt, tar});
					if(trace) prev[tar] = i;
				}
			}
		}
	}
	
	void AuxGraph::label_edge(int src,int dst,int *prev) {
		while(dst != src) {
			std::cout << "EDGE " << src << ' ' << dst << std::endl;
			edges[prev[src]].J ^= 1;
			src = edges[prev[src]].getAdj(src);
		}
	}

	std::vector<std::pair<int,int>> AuxGraph::getJ() {
		std::vector<std::pair<int,int>> ret;
		for(auto &e: edges) if(e.J) ret.push_back({e.u, e.v});
		return std::move(ret);
	}

	void calculate_dist(int n, int m, double *cost, double *dist) {
		const double INF = 1e100;
		for(int i=0;i<n;i++) for(int j=0;j<n;j++)
			dist[i*n + j] = (i==j ? 0 : INF);

		for(int i=0;i<n;i++) for(int j=i+1;j<n;j++) for(int k=0;k<m;k++) {
			dist[i*n+j] = std::min(dist[i*n+j], cost[i*m+k] + cost[j*m+k]);
			dist[j*n+i] = std::min(dist[j*n+i], cost[i*m+k] + cost[j*m+k]);
		}
	}

	void solve_join(AuxGraph &G, int n, std::vector<int> &T) {
		double *cost = new double[T.size() * T.size()];
		double *dist = new double[n];
		for(int i=0;i<T.size();i++) {
			G.shortestPath(T[i], dist, nullptr);
			for(int j=0;j<T.size();j++) cost[i*T.size()+j] = dist[T[j]];
		}

		PerfectMatching pm(T.size(), T.size() * (T.size()-1) / 2);
		pm.options.fractional_jumpstart = true;
		pm.options.dual_greedy_update_option = 0;
		pm.options.dual_LP_threshold = 0.00;
		pm.options.update_duals_before = false;
		pm.options.update_duals_after = false;
		pm.options.single_tree_threshold = 1.00;
		pm.options.verbose = false;

		for(int i=0;i<T.size();i++) for(int j=i+1;j<T.size();j++)
			pm.AddEdge(i, j, cost[i*T.size() + j]);
		
		std::cerr << "PM Start" << std::endl;
		pm.Solve();
		std::cerr << "PM END" << std::endl;

		std::cerr << "T vertex " << n << std::endl;
		for(auto &t: T)
			std::cerr << t << ' ' ;
		std::cerr << std::endl << std::endl;

		int c=0, *prev = new int[n];
		for(int i=0;i<T.size();i++) {
			int j = pm.GetMatch(i);
			if(i<j) {
				std::cerr << "MATCHING " << T[i] << ' ' << T[j] << std::endl;
				G.shortestPath(T[i], dist, prev, true);
				G.label_edge(T[j], T[i], prev);
			}
		}

		delete []cost;
	}

	void sparsify(AuxGraph &G, AuxGraph &ret, int n, double *open, int *sub_tar, int m) {
		std::vector<std::set<int>> adj(n);
		std::vector<std::pair<int, int>> p = G.getJ();

		for(auto &e: p) {
			std::cerr << e.first << ' ' << e.second << std::endl;
			adj[e.first].insert(e.second);
			adj[e.second].insert(e.first);
		}
		
		std::set<int> cand;
		for(int i=0;i<n;i++) if(adj[i].size() > 1) cand.insert(i);
		while(!cand.empty()) {
			bool f = false;
			for(auto cur: cand) {
				for(auto u: adj[cur]) {
					for(auto v: adj[cur]) if(u!=v) {
						if(adj[u].find(v) != adj[u].end()) {
							adj[u].erase(v); adj[v].erase(u);
							f = true;
						} else if(cur<n-1 && u<n-1 && v<n-1) {
							adj[u].insert(v); adj[v].insert(u);
							f=true;
						} else if(cur == n-1) {
							if(u<m && open[u]>0.5 && sub_tar[u] == v) {
								adj[u].insert(v); adj[v].insert(u);
								f=true;
							} else if(v<m && open[v]>0.5 && sub_tar[v] == u) {
								adj[u].insert(v); adj[v].insert(u);
								f=true;
							}
						}
						
						if(f) {
							cand.erase(cur); cand.erase(u); cand.erase(v);
							adj[cur].erase(u); adj[cur].erase(v);
							adj[u].erase(cur); adj[v].erase(cur);

							if(adj[cur].size() > 1) cand.insert(cur);
							if(adj[u].size() > 1) cand.insert(u);
							if(adj[v].size() > 1) cand.insert(v);
							break;
						}
					}
					if(f) break;
				}
				if(f) break;
			}
			if(!f) break;
		}

		for(int u=0;u<n;u++) for(auto &v:adj[u]) if(u<v) {
			std::cerr << "ABSTRACT " << u << ' ' << v << std::endl;
			ret.addEdge(u, v, 0);
		}
	}

	void reassign(AuxGraph &G, int V, int n, int m, double *_open, double *_assign,
			int* sub_tar, int *refinv) {
		std::unique_ptr<double[]> open(new double[V]), assign(new double[V*m]);
		memset(open.get(), 0, sizeof(double)*V);
		memcpy(open.get(), _open, sizeof(double)*n);

		for(int i=0;i<n;i++) for(int j=0;j<m;j++) 
			assign[i*m+j] = _assign[i*m+j];
		for(int i=n;i<V;i++) for(int j=0;j<m;j++)
			assign[i*m+j] = 0;

		std::set<std::pair<int,int>> edges;
		for(auto &e:G.edges) {
			int u = e.u, v = e.v;
			if(u>v) std::swap(u, v);
			edges.insert({u, v});
		}
		std::cerr << "Reassign Construct" << std::endl;

		// open 
		for(auto &i:G.adj[V-1]) {
			int v = G.edges[i].getAdj(V-1);
			if(open[v] < 0.5) open[v] = 1, edges.erase({v, V-1});
		}

		std::cerr << "R - Open" << std::endl;
		// move one clinet 
		for(auto &e:G.edges) if(e.u!=V-1 && e.v!=V-1) {
			int u = e.u, v = e.v;
			
			auto func = [&](int u,int v) { 
				int tar = -1;
				for(int j=0;j<m;j++) if(assign[u*m + j] > 0.5) {
					tar = j;
					break;
				}
				if(tar==-1) return false;
				
				std::cerr << "Reassign from " << u << " to " << v << ' ' << tar << std::endl;
				assign[u*m + tar] = 0;
				assign[v*m + tar] = 1;
				return true;
			};
			if(edges.find({u, V-1}) != edges.end()) 
				assert(func(u, v));
			else if(edges.find({v, V-1}) != edges.end()) 
				assert(func(v, u));
			else {
				if(!func(u,v)) assert(func(v, u));
			}

			edges.erase({std::min(u,v), std::max(u,v)});
		}
		std::cerr << "R - move" << std::endl;

		// close
		for(auto &e:edges) {
			int u = e.first, v = e.second;
			if(u>v) std::swap(u,v);

			for(int j=0;j<m;j++) if(assign[u*m + j] > 0.5) {
				assign[sub_tar[u]*m + j] = 1;
				assign[u*m + j] = 0;
			}
			open[u] = 0;
			open[sub_tar[u]] = 1;
		}
		std::cerr << "R - end" << std::endl;

		for(int i=0;i<n;i++) {
			_open[i] = open[i];
			for(int j=0;j<m;j++)
				_assign[i*m + j] = assign[i*m + j];
		}

		for(int i=n;i<V-1;i++) {
			_open[refinv[i-n]] = (_open[refinv[i-n]]>0.5 || open[i] > 0.5 ? 1 : 0);
			for(int j=0;j<m;j++) {
				_assign[refinv[i-n]*m + j] = (_assign[refinv[i-n]*m+j]>0.5 || assign[i*m+j]>0.5 ? 1 : 0);
			}
		}
	}

	void approximate_unconstraint(PCFLProbData *data, double *_open, double *_assign) {
		// n: # of facility, m: # of client
		int n = data->m, m = data->n;

		int unconstr_cnt = 0, *refnum = new int[n], *refinv = new int[n];
		for(int i=0;i<n;i++) if(data->parity[i] == 0) {
			refnum[i] = unconstr_cnt;
			refinv[unconstr_cnt++] = i;
		}

		int V = n + unconstr_cnt;
		int *assign_cnt = new int[V];
		memset(assign_cnt, 0, sizeof(int)*V);

		for(int i=0;i<n;i++) {
			for(int j=0;j<m;j++) if(_assign[i*m+j] > 0.5) assign_cnt[i]++;
		}
		
		// unconstrained -> odd or even
		int *parity = new int[V];
		memcpy(parity, data->parity, sizeof(int)*n);
		for(int i=0;i<n;i++) if(parity[i]==0) {
			if(assign_cnt[i]%2==0)
				parity[i] = 2, parity[n+refnum[i]] = 1;
			else 
				parity[i] = 1, parity[n+refnum[i]] = 2;
		}

		int *open = new int[V];
		for(int i=0;i<n;i++) {
			if(data->parity[i]==0)
				open[i] = _open[i] > 0.5, open[n+refnum[i]] = 0;
			else
				open[i] = _open[i] > 0.5;
		}

		double *_dist = new double[n*n], *dist = new double[V*V];
		calculate_dist(n, m, data->assign_cost, _dist);
		for(int i=0;i<V;i++) for(int j=0;j<V;j++) {
			int u=i<n ? i : refinv[i-n], v = j<n ? j : refinv[j-n];
			dist[i*V+j] = _dist[u*n+v];
		}

		double *openCost = new double[V];
		for(int i=0;i<V;i++)
			openCost[i] = data->opening_cost[i<n ? i : refinv[i-n]];
			
		delete []_dist;
		delete []refnum;
		
		// make graph
		AuxGraph G(V+1);

		// reassign edge
		for(int i=0;i<V;i++) for(int j=i+1;j<V;j++)
			G.addEdge(i, j, dist[i*V + j]);

		// opening edge
		for(int i=0;i<V;i++) if(parity[i]==1 && !open[i])
			G.addEdge(i, V, openCost[i]);
		
		// clsoing edge;
		int open_cnt = 0, even_close_cnt = 0;
		for(int i=0;i<V;i++) {
			if(open[i]) open_cnt++;
			else if(parity[i]==2) even_close_cnt++;
		}

		int *sub_tar = new int[V];
		memset(sub_tar, -1, sizeof(int)*V);
		if(open_cnt >= 2 || even_close_cnt >= 1) {
			for(int i=0;i<V;i++) if(open[i] && parity[i]==1) {
				std::pair<double, int> cost(1e100, -1);
				
				for(int j=0;j<V;j++) if(i!=j) {
					if(open[j]) cost = min(cost, {assign_cnt[j] * dist[i*V + j], j});
					else if(parity[j]==2) cost = min(cost, {assign_cnt[j] * dist[i*V + j] + openCost[j], j});
				}
				sub_tar[i] = cost.second;

				G.addEdge(i, V, cost.first);
			}
		}

		std::cerr << "Opens " << std::endl;
		for(int i=0;i<V;i++) if(open[i]) std::cerr << i << ' ' ;
		std::cerr << std::endl << std::endl;

		std::vector<int> T;
		for(int i=0;i<V;i++) if(open[i] && assign_cnt[i]%2 != parity[i]%2)
			T.push_back(i);
		if(T.size()%2==1) T.push_back(V);

		std::cerr << "Constructing is done" << std::endl;
		solve_join(G, V+1, T);
		std::cerr << "Solve" << std::endl;

		AuxGraph ans(V+1);
		sparsify(G, ans, V+1, _open, sub_tar, n);
		std::cerr << "sparsify done" << std::endl;
		reassign(ans, V+1, n, m, _open, _assign, sub_tar, refinv);
		std::cerr << "reassign done" << std::endl;
		assert(check_validity(data, n, m, _open, _assign));
		
		delete []openCost;
		delete []assign_cnt;
		delete []parity;
		delete []open;
		delete []refinv;
	}

	bool check_validity(PCFLProbData *data, int n,int m,double *open, double *assign) {
		int *acnt = new int[n];
		memset(acnt, 0, sizeof(int) * n);
		for(int i=0;i<n;i++) for(int j=0;j<m;j++) 
			acnt[i] += assign[i*m+j] > 0.5;

		for(int i=0;i<n;i++) {
			if(open[i] < 0.5 && acnt[i]!=0) {
				std::cerr << "ERROR at " << i << " not opend but assigned" << std::endl;
				return false;
			}
			if(open[i] > 0.5 && data->parity[i] && data->parity[i]%2 != acnt[i]%2) {
				std::cerr << "ERROR at " << i << " it isn't satisfy parity " << data->parity[i] << ' ' << acnt[i] << std::endl;
				return false;
			}
		}

		int cnt=0;
		for(int i=0;i<n;i++) cnt += acnt[i];
		if(cnt != m) {
			std::cerr << "ERROR at assign: some clinets weren't assigned" << std::endl;
			return false;
		}

		delete []acnt;

		return true;
	}
}
