#include "SPH_RIDG.h"

//Default Constructor
SPH_RIDG::SPH_RIDG()
{
	J = 2;
	rho = 0.5;
	UM = UtilMath();
	init();
}

SPH_RIDG::~SPH_RIDG() {}

SPH_RIDG::SPH_RIDG(unsigned JIn, precisionType rhoIn) {
	J = JIn;
	rho = rhoIn;
	UM = UtilMath();
	init();
}

void SPH_RIDG::init() {
	mcut = (int)ceil(sqrt(-log(1e-6) * pow(4, J) / rho));
	mcut = mcut + mcut % 2;

	h = MatrixType::Zero(mcut + 1, J + 1);
	h.col(0) = VectorType::LinSpaced(mcut + 1, 0, mcut);

	for (int i = 1; i < J + 1; ++i)
		h.col(i) = h.col(i - 1) * 0.5;

	h = (-rho * h.array().cwiseProduct(h.array() + 1.0)).exp();

	Lmd = MatrixType::Zero(mcut + 1, 1);
	UM.fura(Lmd, mcut);
	psi = h;
	psi.col(0) = psi.col(0).cwiseProduct(Lmd);
	for (int i = 1; i < J + 1; ++i) {
		psi.col(i) = (h.col(i) - h.col(i - 1)).cwiseProduct(Lmd);
	}

	C = MatrixType::Zero(mcut + 1, 1);
	C.col(0) = (2.0 * VectorType::LinSpaced(mcut + 1, 0, mcut).array() + 1.0) / (4 * UM.PI);

	t = ((psi.cwiseProduct(psi)).transpose() * C).array().sqrt();
	for (int i = 0; i < J + 1; ++i)
		psi.col(i) = psi.col(i) / t(i);

	tau = 4.0 * log(10.0) / rho;
	m0 = (int)floor((-1.0 + sqrt(1.0 + 4.0 * tau)) / 2.0);
	M0 = VectorXi::Zero(J + 1, 1);

	for (int i = 0; i < J + 1; ++i)
		M0(i, 0) = (int)pow((pow(2, i) * m0 + 1), 2);
}

void SPH_RIDG::RBasis(MatrixType& A, MatrixType& u) {
	cout << "Start computing R basis..." << endl;
	A.resize(u.rows(), M0.sum());
	A.setZero();

	MatrixType P;
	MatrixType X;
	MatrixType x;

	MatrixType v;
	MatrixType vv;
	MatrixType r;
	int I = 0;
	for (int i = 0; i < J + 1; ++i) {
		int K = M0(i);
		int N = 2 * K;
		v = MatrixType::Zero(N, 3);
		UM.spiralsample(v, 2, N);
		vv = MatrixType::Zero(K, 3);
		vv = v.topRows(K);

		r = C.cwiseProduct(psi.col(i));
		P = MatrixType::Ones(u.rows(), mcut + 1);
		X = u * vv.transpose();
		for (int k = 0; k < K; ++k) {
			x = X.col(k);
			UM.polyleg(P, x, mcut);
			A.col(k + I) = P * r;
		}
		I += K;
	}
}

void SPH_RIDG::QBasis(MatrixType& Q, MatrixType& u) {
	cout << "Start computing Q basis..." << endl;
	Q.resize(u.rows(), M0.sum());
	Q.setZero();

	MatrixType P;
	MatrixType x;

	MatrixType v;
	MatrixType vv;
	MatrixType r;
	MatrixType CL = C.cwiseProduct(Lmd);

	int I = 0;
	for (int i = 0; i < J + 1; ++i) {
		int K = M0(i);
		int N = 2 * K;
		v = MatrixType::Zero(N, 3);
		UM.spiralsample(v, 2, N);
		vv = v.topRows(K);

		r = CL.cwiseProduct(psi.col(i));
		for (int k = 0; k < K; ++k) {
			x = u * vv.row(k).transpose();
			unsigned Nr = (unsigned)x.rows();
			P = MatrixType::Ones(Nr, mcut + 1);
			UM.polyleg(P, x, mcut);

			Q.col(k + I) = P * r;
		}
		I += K;
	}
}

void SPH_RIDG::normBasis(MatrixType& mat) {
	MatrixType e = mat * mat.transpose();
	SelfAdjointEigenSolver<MatrixType> eigensolver(mat.rows());
	eigensolver.compute(e, EigenvaluesOnly);
	precisionType lVal = eigensolver.eigenvalues()[mat.rows() - 1];
	mat = (1 / sqrt(lVal)) * mat;
}