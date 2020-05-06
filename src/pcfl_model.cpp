#include "pcfl_model.h"

/* Model Definition */
PCFLModel::PCFLModel(double _timeLimit=30.0)
	:m_timeLimit(_timeLimit), m_env(true), m_openVar(nullptr), m_assignVar(nullptr), m_parityVar(nullptr), GRBModel(false), m_parityConstr(nullptr)
{
}
PCFLModel::PCFLModel(const GRBEnv &env, double _timeLimit=30.0)
	:m_timeLimit(_timeLimit), m_env(true), m_openVar(nullptr), m_assignVar(nullptr), m_parityVar(nullptr), GRBModel(env), m_parityConstr(nullptr)
{
}

void PCFLModel::constructModel(const ProbData &d) {
	// Base constraint
	nrFacility = d.nrFacility;
	nrClient = d.nrClient;
	m_openVar = makeOpeningVars(d);
	m_assignVar = makeAssigningVars(d);
	addConstr_AssignWithinCap(d, m_openVar, m_assignVar);
	addConstr_AssignOnlyOnce(d, m_assignVar);
	m_parityVar = makeConstr_Parity(d, m_openVar, m_assignVar);
	
	// extra constarint
	if(PCFLModelSetter::getInstance().m_iDistAssignRatio) {
		addConstr_DistAssign(d, m_openVar, m_assignVar);
	}

	// set callback and related one
	PCFLCallback *cb = new PCFLCallback(nrFacility, nrClient, m_openVar, m_assignVar, m_parityVar);
	if(PCFLModelSetter::getInstance().getAssignLazy()) {
		cb->setAssignOnceConstr(m_assignConstr);
		set(GRB_IntParam_LazyConstraints, 1);
	}
	if(PCFLModelSetter::getInstance().getLazyConstr()) {
		cb->setLazyConstr(PCFLModelSetter::getInstance().getLazyConstr());
		set(GRB_IntParam_LazyConstraints, 1);

		if(getFlag(PCFLModelSetter::getInstance().getLazyConstr(), LAZY_PARITY_CONSTR))
			cb->setParityExpr(m_parityConstr);
	}
	cb->activateTrace(PCFLModelSetter::getInstance().m_sTraceDir);
	cb->init();
	setCallback(cb);

	// set some extra condition for IP
	PCFLModelSetter::getInstance().setModelProp(*this);
	set(GRB_DoubleParam_MIPGap, 0);
}

void PCFLModel::setVariableProperties() {
	set(GRB_IntParam_Threads, std::thread::hardware_concurrency());
	set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
	if(m_timeLimit >= 0) set(GRB_DoubleParam_TimeLimit, m_timeLimit);
	set(GRB_DoubleParam_MIPGap, 0);

	GRBCallback *cb = new PCFLCallback(nrFacility, nrClient, m_openVar, m_assignVar, m_parityVar);
	setCallback(cb);
}

void PCFLModel::run() {
	optimize();
	if(PCFLModelSetter::getInstance().getDeferConstr()) {
		int defer_flag = PCFLModelSetter::getInstance().getDeferConstr();
		if(getFlag(defer_flag, DEFER_PARITY_CONSTR)) {
			addConstr_Parity();
			defer_initGuess();
		}

		optimize();
	}
}

PCFLModel::~PCFLModel() {
	if(m_openVar) 
		delete []m_openVar;

	if(m_parityVar)
		delete []m_parityVar;
		

	if(m_assignVar) {
		for(int i=0;i<nrFacility;i++) delete []m_assignVar[i];
		delete []m_assignVar;
	}

	if(m_assignConstr)
		delete []m_assignConstr;

	if(m_parityConstr) 
		delete []m_parityConstr;
}
	

// make facility and thier objective.
GRBVar* PCFLModel::makeOpeningVars(const ProbData &d) {
	GRBVar* open = addVars(d.nrFacility, GRB_BINARY);
	for(int i=0; i<d.nrFacility; i++) {
		// name of varaible
		ostringstream vname;
		vname << "y" << i;
		
		// set Object
		open[i].set(GRB_DoubleAttr_Obj, d.openCost[i]);
		open[i].set(GRB_StringAttr_VarName, vname.str());
	}
	return open;
}

// mae client and thier objective.
GRBVar** PCFLModel::makeAssigningVars(const ProbData &d) {
	GRBVar** assign = new GRBVar* [d.nrFacility];
	for(int i=0; i<d.nrFacility; i++) assign[i] = addVars(d.nrClient, GRB_BINARY);
	for(int j=0; j<d.nrClient; j++) {
		for(int i=0; i<d.nrFacility; i++) {
			// name of variable
			ostringstream vname;
			vname << "x(" << i << ',' << j << ")";
			
			// set objective
			assign[i][j].set(GRB_DoubleAttr_Obj, d.assignCost[i][j]);
			assign[i][j].set(GRB_StringAttr_VarName, vname.str());
		}
	}
	return assign;
}

void PCFLModel::addConstr_AssignWithinCap(const ProbData &d, const GRBVar *open, GRBVar **assign) {
	for(int i=0; i<d.nrFacility; i++) {
		for(int j=0; j<d.nrClient; j++) {
			ostringstream cname;
			cname << "x(" << i << ',' << j << ")<=y" << i;
			addConstr(assign[i][j] <= open[i], cname.str());
		}
	}
}
void PCFLModel::addConstr_AssignOnlyOnce(const ProbData &d,	GRBVar **assign) {
	m_assignConstr = new GRBLinExpr[d.nrClient];
	for(int j=0; j<d.nrClient; j++) {
		GRBLinExpr &sum = m_assignConstr[j];
		for(int i=0; i<d.nrFacility; i++) {
			sum += assign[i][j];
		}
		ostringstream cname;
		cname << "sum(x(_," << j << "))=1";

		if(PCFLModelSetter::getInstance().getAssignLazy())
			addConstr(sum >= 1, cname.str());
		else
			addConstr(sum == 1, cname.str());
	}
}
GRBVar* PCFLModel::makeConstr_Parity(const ProbData &d, GRBVar *open, GRBVar **assign) {
	GRBVar *cap = addVars(d.nrFacility, GRB_INTEGER);
	for(int i=0; i<d.nrFacility; i++) {
		cap[i].set(GRB_DoubleAttr_Obj, 0);
		cap[i].set(GRB_DoubleAttr_LB, 0);
	}

	m_parityConstr = new GRBTempConstr[d.nrFacility];
	for(int i=0; i<d.nrFacility; i++) {
		if(d.parityConstr[i]!=0) {
			GRBLinExpr sum = 0;
			for(int j=0; j<d.nrClient; j++) {
				sum += assign[i][j];
			}
			m_parityConstr[i] = (d.parityConstr[i] == 1) ? 
				(sum == 2*cap[i] + open[i]) :
				(sum == 2*cap[i]);
		}
	}
	if(!getFlag(PCFLModelSetter::getInstance().getDeferConstr(), DEFER_PARITY_CONSTR) &&
			!getFlag(PCFLModelSetter::getInstance().getState(), STATE_UNCONSTRAINED) )
		addConstr_Parity();
	return cap;
}

void PCFLModel::addConstr_Parity() {
	vector<int> &parity_constr = PCFLUtility::getInstance().getInput()->parityConstr;
	for(int i=0; i<nrFacility; i++) if(parity_constr[i]) {
		ostringstream cname;
		cname << "sum(x(" << i << ",_))==2*z" << i << "+a";
		GRBConstr tmp = addConstr(m_parityConstr[i], cname.str());

		if(getFlag(PCFLModelSetter::getInstance().getLazyConstr(), LAZY_PARITY_CONSTR))
			tmp.set(GRB_IntAttr_Lazy, 2);
	}
}

void PCFLModel::addConstr_DistAssign(const ProbData &d, GRBVar *openVar, GRBVar **assignVar) {
	int ratio = d.nrFacility / PCFLModelSetter::getInstance().m_iDistAssignRatio;
	const vector<vector<double>> &dist = d.assignCost;
	const vector<int> &parity = d.parityConstr;

	for(int i=0; i<d.nrFacility; i++) {
		vector<pair<double, int>> facs(d.nrFacility);
		for(int k=0; k<d.nrFacility; k++)
			facs[k] = {dist[i][0] + dist[k][0], k};
		
		for(int k=0; k<d.nrFacility; k++) {
			for(int j=1; j<d.nrClient; j++)
				facs[k].first = min(facs[k].first, dist[i][j] + dist[k][j]);
		}
		sort(facs.begin(), facs.end(), std::greater<pair<double, int>>());
		facs.resize(ratio);
		
		vector<int> shorter_clients;
		for(int j=0; j<d.nrClient; j++) {
			bool flag = true;
			for(auto &k: facs) if(dist[i][j] >= dist[k.second][j]) {
				flag = false;
				break;
			}
			if(flag) shorter_clients.push_back(j);
		}

		GRBLinExpr sum = 0;
		int DS = shorter_clients.size(), SS = 0;
		for(auto &client: shorter_clients) {
			for(auto &k: facs) sum += assignVar[k.second][client];
		}
		for(auto &k: facs) if(parity[k.second] != 0) ++SS;
		if(parity[i] != 0) ++SS;
		addConstr(sum <= DS - (DS - SS)*openVar[i]);
	}
}

void PCFLModel::defer_initGuess() {
	vector<double> fac(nrFacility);
	vector<vector<double>> ass(nrFacility, vector<double>(nrClient));
	for(int i=0; i<nrFacility; i++) 
		fac[i] = m_openVar[i].get(GRB_DoubleAttr_X);
	for(int i=0; i<nrFacility; i++) for(int j=0; j<nrClient; j++)
		ass[i][j] = m_assignVar[i][j].get(GRB_DoubleAttr_X);

	ProbData *data = PCFLUtility::getInstance().getInput();
	vector<int> &par = data->parityConstr;
	vector<double> &oc = data->openCost;
	vector<vector<double>> &ac = data->assignCost;
	vector<int> inv;
	
	for(int i=0; i<nrFacility; i++) if(fac[i] > 0.5 && par[i]) {
		int cnt=0;
		for(int j=0; j<nrClient; j++) if(ass[i][j] > 0.5) cnt++;
		if(cnt%2 != par[i]%2) inv.push_back(i);
	}

	if(!inv.empty()) {
		const double INF = 1e100;
		double min_cost = INF;
		int min_fac = -1;
		vector<pair<int, int>> rea;
		for(int i=0; i<nrFacility; i++) if(fac[i] < 0.5 && (!par[i] || inv.size()%2==par[i]%2)) {
			double cost_sum = 0;
			vector<pair<int, int>> r;
			for(auto &f:inv) {
				double mc = INF;
				int mj = -1;
				for(int j=0; j<nrClient; j++) if(ass[f][j] > 0.5 && mc > ac[i][j] - ac[f][j]) {
					mc = ac[i][j] - ac[f][j];
					mj = j;
				}
				cost_sum += mc;
				r.push_back({f, mj});
			}
			if(min_cost > cost_sum) {
				min_cost = cost_sum;
				min_fac = i;
				rea = r;
			}
		}

		if(min_fac!=-1) {
			cout << "Find Valid Solution" << endl;
			fac[min_fac] = 1.0;
			for(auto &v:rea) {
				ass[v.first][v.second] = 0.0;
				ass[min_fac][v.second] = 1.0;
			}
		}
	}
	for(int i=0; i<nrFacility; i++) m_openVar[i].set(GRB_DoubleAttr_Start, fac[i]);
	for(int i=0; i<nrFacility; i++) for(int j=0; j<nrClient; j++)
		m_assignVar[i][j].set(GRB_DoubleAttr_Start, ass[i][j]);
}

/* Setter definition */
PCFLModelSetter *PCFLModelSetter::instance = nullptr;
int PCFLModelSetter::m_iOpenPrior = 0;
int PCFLModelSetter::m_iAssignPrior = 0;
int PCFLModelSetter::m_iParityPrior = 0;

bool PCFLModelSetter::m_bAssignLazy = false;

int PCFLModelSetter::m_iLazyConstr = 0;
int PCFLModelSetter::m_iDeferConstr = 0;
int PCFLModelSetter::m_iState = 0;

int PCFLModelSetter::m_iDistAssignRatio = 0;

string PCFLModelSetter::m_sTraceDir = "";

PCFLModelSetter::PCFLModelSetter()
{
}

PCFLModelSetter& PCFLModelSetter::getInstance() {
	if(!instance)
		instance = new PCFLModelSetter();

	return *instance;
}

void PCFLModelSetter::setModelProp(PCFLModel &model) {
	do_openPrior(model);
	do_assignPrior(model);
	do_parityPrior(model);
}

void PCFLModelSetter::do_openPrior(PCFLModel &model) {
	if(!m_iOpenPrior) return;

	for(int i=0; i<model.nrFacility; i++) {
		model.m_openVar[i].set(GRB_IntAttr_BranchPriority, m_iOpenPrior);
	}
}
void PCFLModelSetter::do_assignPrior(PCFLModel &model) {
	if(!m_iAssignPrior) return;

	for(int i=0; i<model.nrFacility; i++) {
		for(int j=0; j<model.nrClient; j++) {
			model.m_assignVar[i][j].set(GRB_IntAttr_BranchPriority, m_iAssignPrior);
		}
	}
}
void PCFLModelSetter::do_parityPrior(PCFLModel &model) {
	if(!m_iParityPrior) return;

	for(int i=0; i<model.nrFacility; i++) {
		model.m_parityVar[i].set(GRB_IntAttr_BranchPriority, m_iParityPrior);
	}
}

void PCFLModelSetter::setOpenPrior(int _val) {
	m_iOpenPrior = _val;
}
int PCFLModelSetter::getOpenPrior() {
	return m_iOpenPrior;
}
void PCFLModelSetter::setAssignPrior(int _val) {
	m_iAssignPrior = _val;
}
int PCFLModelSetter::getAssignPrior() {
	return m_iAssignPrior;
}
void PCFLModelSetter::setParityPrior(int _val) {
	m_iParityPrior = _val;
}
int PCFLModelSetter::getParityPrior() {
	return m_iParityPrior;
}

void PCFLModelSetter::setAssignLazy(bool _val) {
	m_bAssignLazy = _val;
}
bool PCFLModelSetter::getAssignLazy() {
	return m_bAssignLazy;
}

void PCFLModelSetter::setLazyConstr(int f) {
	setFlag(m_iLazyConstr, f);
}
void PCFLModelSetter::unsetLazyConstr(int f) {
	unsetFlag(m_iLazyConstr, f);
}
int PCFLModelSetter::getLazyConstr() {
	return m_iLazyConstr;
}

void PCFLModelSetter::setDeferConstr(int f) {
	setFlag(m_iDeferConstr, f);
}
void PCFLModelSetter::unsetDeferConstr(int f) {
	unsetFlag(m_iDeferConstr, f);
}
int PCFLModelSetter::getDeferConstr() {
	return m_iDeferConstr;
}

void PCFLModelSetter::setState(int f) {
	setFlag(m_iState, f);
}
void PCFLModelSetter::unsetState(int f) {
	unsetFlag(m_iState, f);
}
int PCFLModelSetter::getState() {
	return m_iState;
}

void PCFLModelSetter::addConstr(int type, int value) {
	switch(type) {
	case DIST_BASE_CONSTR:
		m_iDistAssignRatio = value;
		break;
	}
}

void PCFLModelSetter::setTraceDir(string dir) {
	m_sTraceDir = dir;
}
