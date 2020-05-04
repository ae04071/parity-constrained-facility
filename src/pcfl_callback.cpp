#include <pcfl_model.h>
#include <cmath>
#include <memory>

const double EPS = 1e-6;
static bool eq(double a, double b)
{
    return fabs(a - b) < EPS;
}

static const char *mapping_callback_state(int where) {
	switch(where) {
		case GRB_CB_POLLING:
			return "PULLING";
		case GRB_CB_PRESOLVE:
			return "PRESOLVE";
		case GRB_CB_SIMPLEX:
			return "SIMPLEX";
		case GRB_CB_MIP:
			return "MIP";
		case GRB_CB_MIPSOL:
			return "MIPSOL";
		case GRB_CB_MIPNODE:
			return "MIPNODE";
		case GRB_CB_BARRIER:
			return "BARRIER";
		default:
			return "NON OF THEM";
	}
}

PCFLModel::Callback::Callback(PCFLModel *_model) : model(_model) {}

void PCFLModel::Callback::callback() {
//	static double *x[1000] = {nullptr, };
//	if(where!=0 && where != 6 && where != 1)
//		cerr << "Current callback state: " << mapping_callback_state(where) << ' ' << where << endl;
    static int counter = 0;
	try {
	    switch (where) {
	        case GRB_CB_POLLING:
                // Ignore polling callback
                break;
            case GRB_CB_PRESOLVE:
                // Presolve callback
                break;
            case GRB_CB_SIMPLEX:
                // Simplex callback
                break;
            case GRB_CB_MIP:
                // General MIP callback
//                {
//                    cerr.unsetf(ios_base::floatfield);
//                    cerr << "MIP"
//                         << ", NODCNT: " << getDoubleInfo(GRB_CB_MIP_NODCNT)
//                         << ", NODLFT: " << getDoubleInfo(GRB_CB_MIP_NODLFT)
//                         << ", OBJBST: " << getDoubleInfo(GRB_CB_MIP_OBJBST)
//                         << ", OBJBND: " << getDoubleInfo(GRB_CB_MIP_OBJBND)
//                         << endl;
//                }

                break;
            case GRB_CB_MIPSOL:
                // MIP solution callback

                if (this->model->config.verbose) {
                    printf("SOLUTION\n");
                    printf(" %.6lf\n", this->getDoubleInfo(GRB_CB_MIPSOL_OBJ));
                    const int m = this->model->data.m, n = this->model->data.n;
                    std::unique_ptr<double[]> y(this->getSolution(model->yvar, m));
                    for (int i = 0; i < m; i++) {
                        if (eq(y[i], 1)) {
                            std::unique_ptr<double[]> x(this->getSolution(
                                    this->model->xvar + (i * n), n));
                            double sum = 0;
                            for (int j = 0; j < n; j++)
                                sum += x[j];
                            printf(" %d(%d, %lf): ", i, this->model->data.parity[i], sum);
                            bool first = true;
                            for (int j = 0; j < n; j++) {
                                if (eq(x[j], 1)) {
                                    printf(first ? "%d" : ", %d", j);
                                    first = false;
                                }
                            }
                            printf("\n");
                        }
                    }
                }

                {
                    const int m = this->model->data.m, n = this->model->data.n;
                    const int *const parity = this->model->data.parity;
                    std::unique_ptr<double[]> y(this->getSolution(this->model->yvar, m));
                    std::unique_ptr<double[]> x(this->getSolution(this->model->xvar, m * n));
                    if (this->model->config.lazy_parity) {
                        for (int i = 0; i < m; i++) {
                            int cur_parity = parity[i];
                            if (y[i] >= 0.5 && cur_parity != 0) {
                                double *x_cur = &x[i * n];
                                double sum = cur_parity;
                                for (int j = 0; j < n; j++)
                                    sum += x_cur[j];
                                if (!eq(fmod(sum, 2), 0)) {
                                    // parity violation
                                    this->addLazy(model->genParityConstr(i));
                                }
                            }
                        }
                    }

                    if (this->model->config.lazy_open) {
                        for (int i = 0; i < m; i++) {
                            if (y[i] < 0.5) {
                                double *x_cur = &x[i * n];
                                for (int j = 0; j < n; j++) {
                                    if (x_cur[j] >= 0.5) {
                                        this->addLazy(model->genOpenConstr(i, j));
                                        counter++;
                                    }
                                }
                            }
                        }
                    }
                }
                printf("counter: %d\n", counter);

                break;
            case GRB_CB_MIPNODE:
                // MIP node callback
//                {
//                    printf("MIPNODE\n");
//                    if (this->getIntInfo(GRB_CB_MIPNODE_STATUS) == GRB_OPTIMAL) {
////                        printf(" %.6lf\n", this->getDoubleInfo(GRB_CB_MIPNODE_OBJ));
//                        const int m = this->model->data.m, n = this->model->data.n;
//                        std::unique_ptr<double[]> y(this->getNodeRel(model->yvar, m));
//                        for (int i = 0; i < m; i++) {
//                            if (eq(y[i], 1)) {
//                                std::unique_ptr<double[]> x(this->getNodeRel(
//                                        this->model->xvar + (i * n), n));
//                                double sum = 0;
//                                for (int j = 0; j < n; j++)
//                                    sum += x[j];
//                                printf(" %d(%d, %lf): ", i, this->model->data.parity[i], sum);
//                                bool first = true;
//                                for (int j = 0; j < n; j++) {
//                                    if (eq(x[j], 1)) {
//                                        printf(first ? "%d" : ", %d", j);
//                                        first = false;
//                                    }
//                                }
//                                printf("\n");
//                            }
//                        }
//                    } else {
//                        printf(" not optimal\n");
//                    }
//                }

//                cout << "######################### MIPNODE INFO #####################################" << endl;
//                {
//                    double *x = getSolution(openVar, nrFacility);
//                }
//                for(int i=0; i<nrFacility; i++) {
//                    double lb = openVar[i].get(GRB_DoubleAttr_LB);
//                    double ub = openVar[i].get(GRB_DoubleAttr_UB);
//                    if(lb > 0.1 || ub < 0.9)
//                        cout << "Bound was Changed: Facility " << i << ", (" << lb << ", " << ub << ")" << endl;
//                }
//                for(int i=0; i<nrFacility; i++) {
//                    for(int j=0; j<nrClient; j++) {
//                        double lb = assignVar[i][j].get(GRB_DoubleAttr_LB);
//                        double ub = assignVar[i][j].get(GRB_DoubleAttr_UB);
//                        if(lb > 0.1 || ub < 0.9)
//                            cout << "Bound was Changed: Assign " << j << " to " << i << ", (" << lb << ", " << ub << ")" << endl;
//                    }
//                }
//                for(int i=0; i<nrFacility; i++) {
//                    double lb = parityVar[i].get(GRB_DoubleAttr_LB);
//                    double ub = parityVar[i].get(GRB_DoubleAttr_UB);
//                    if(lb > 0.1 || ub < 0.9)
//                        cout << "Bound was Changed: Facility " << i << ", (" << lb << ", " << ub << ")" << endl;
//                }
//                cout << endl;
//    			cout << "Start Compare" << endl;
//    			cout << fixed;
//    			cout.precision(1);
//    			for(int i=0; i<nrFacility; i++) {
//    				double *y = getNodeRel(assignVar[i], nrClient);
//    				if(x[i]) {
//    					for(int j=0; j<nrClient; j++) if(!eq(x[i][j], y[j]))
//    						cout << "DIFF " << i << ' ' << j << " " << fabs(x[i][j]) << ' ' << fabs(y[j]) << endl;;
//    					delete []x[i];
//    				}
//    				x[i] = y;
//    			}
//
//    			cout << "# of cuts: " << getIntInfo(GRB_CB_MIPNODE_STATUS) << endl;

//                {
//                    cerr << "MIPNODE_STATUS: " << getIntInfo(GRB_CB_MIPNODE_STATUS) << endl;
//                    double *open_val = getNodeRel(openVar, nrFacility);
//                    if (open_val) {
//                        cerr << "Open:";
//                        for (int i = 0; i < nrFacility; i++) {
//                            cerr << " " << open_val[i];
//                        }
//                        cerr << endl;
//                    } else {
//                        cerr << "Unavailable" << endl;
//                    }
//                }
//                {
//                    cerr.unsetf(ios_base::floatfield);
//                    cerr << "MIPNODE"
//                         << ", NODCNT: " << getDoubleInfo(GRB_CB_MIPNODE_NODCNT)
////                         << ", NODLFT: " << getDoubleInfo(GRB_CB_MIPNODE_NODLFT)
//                         << ", OBJBST: " << getDoubleInfo(GRB_CB_MIPNODE_OBJBST)
//                         << ", OBJBND: " << getDoubleInfo(GRB_CB_MIPNODE_OBJBND)
//                         << endl;
//                }
                break;
            case GRB_CB_BARRIER:
                // Barrier callback
                break;
            case GRB_CB_MESSAGE:
                // Message callback
                break;
	        default:
	            break;
		}
	} catch (const GRBException &e) {
	    fprintf(stderr, "Error code: %d\n%s\n",
                e.getErrorCode(), e.getMessage().c_str());
	} catch (...) {
		fprintf(stderr, "Error during callback\n");
	}
}
