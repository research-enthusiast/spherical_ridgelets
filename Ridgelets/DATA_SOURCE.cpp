#include "pch.h"
#include "DATA_SOURCE.h"

int DATA_SOURCE::CLI(int argc, char* argv[], input_parse& output) {
	if (argc < 5)
	{
		cerr << "Usage: Ridgelets -i dMRI_file and at least one output: -r, -q, -o, -f, -v" << endl;
		cerr << "Optional input arguments: -m mask_file" << endl;
		cerr << "Possible output argumet(s): -r ridgelet_file -q Q coefficients -o ODF_values -f fiber_directions -v fiber volume" << endl;
		return EXIT_FAILURE;
	}

	bool inp1 = false;
	bool out1 = false;
	for (int i = 0; i < argc; ++i) {
		if (!strcmp(argv[i], "-i")) {
			output.input_dmri = argv[i + 1];
			inp1 = true;
		}
		if (!strcmp(argv[i], "-m")) {
			output.input_mask = argv[i + 1];
		}
		if (!strcmp(argv[i], "-r")) {
			output.output_ridgelets = argv[i + 1];
			out1 = true;
		}
		if (!strcmp(argv[i], "-q")) {
			output.output_qs = argv[i + 1];
			out1 = true;
		}
		if (!strcmp(argv[i], "-f")) {
			output.output_fiber_dirs = argv[i + 1];
			out1 = true;
		}
		if (!strcmp(argv[i], "-v")) {
			output.output_fiber_volumes = argv[i + 1];
			out1 = true;
		}
	}
	if (!inp1 || !out1) {
		cerr << "Please, provide at least one input AND one output file names" << endl;
		return EXIT_FAILURE;
	}
	return 0;
}

int DATA_SOURCE::readVolume(
	string inputVolume, MatrixType &GradientDirections, DiffusionImagePointer &image,
	unsigned &nGradImgs, unsigned &nOfImgs) {
	bool is_b0 = false;
	double b0 = 0;
	double x, y, z;

	// Temporary register factories cause don't use cmake
	itk::NrrdImageIOFactory::RegisterOneFactory();

	// Another way to store image data
	ImageReaderType::Pointer reader = ImageReaderType::New();

	// Make some inputfiles checks
	string ext_vol = inputVolume.substr(inputVolume.length() - 4, inputVolume.length());

	if (ext_vol.compare("nhdr")) {
		if (ext_vol.compare("nrrd")) {
			cout << "NDHR or NRRD file formats only! Please, check file type." << endl;
			return EXIT_FAILURE;
		}
	}
	if (!is_path_exists(inputVolume)) {
		cout << "Input dMRI image is not exists! Please, check the path and file name." << endl;
		return EXIT_FAILURE;
	}

	// Get image
	reader->SetFileName(inputVolume);
	try
	{
		reader->Update();
		image = reader->GetOutput();
	}
	catch (itk::ExceptionObject)
	{
		cerr << "Can't read input dMRI file! Please, check that file is not corrupted." << endl;
		return EXIT_FAILURE;
	}

	// Get and process header information
	itk::MetaDataDictionary imgMetaDictionary = image->GetMetaDataDictionary();
	vector<string> imgMetaKeys = imgMetaDictionary.GetKeys();
	vector<string>::const_iterator itKey = imgMetaKeys.begin();
	string metaString;

	for (; itKey != imgMetaKeys.end(); ++itKey)
	{
		itk::ExposeMetaData<string>(imgMetaDictionary, *itKey, metaString);
		if (itKey->find("DWMRI_gradient") != string::npos)
		{
			//cout << *itKey << " -> " << metaString << endl;
			sscanf(metaString.c_str(), "%lf %lf %lf\n", &x, &y, &z);

			++nOfImgs;
			// If the direction is 0.0, this is a reference image
			if (x == 0.0 && y == 0.0 && z == 0.0)
				continue;

			GradientDirections.conservativeResize(GradientDirections.rows() + 1, GradientDirections.cols());
			GradientDirections.row(GradientDirections.rows() - 1) << x, y, z;
			++nGradImgs;
		}
		else if (itKey->find("DWMRI_b-value") != string::npos)
		{
			//cout << *itKey << " -> " << metaString << endl;
			is_b0 = true;
			b0 = stod(metaString.c_str());
		}
	}
	// Normalize directions
	GradientDirections.rowwise().normalize();

	cout << "Number of gradient images: " << nGradImgs << ". Number of reference images: " << nOfImgs - nGradImgs << endl;
	cout << "b-value " << b0 << endl;
	if (!is_b0)
	{
		cerr << "b-value not specified in file's header." << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int DATA_SOURCE::readMask(string inputMask, MaskImagePointer& image) {
	// Temporary register factories cause don't use cmake
	itk::NrrdImageIOFactory::RegisterOneFactory();

	// Another way to store image data
	MaskReaderType::Pointer reader = MaskReaderType::New();

	// Make some inputfiles checks
	string ext_vol = inputMask.substr(inputMask.length() - 4, inputMask.length());

	if (!is_path_exists(inputMask)) {
		cout << "Input mask image is not provided. Please, stop program and provide mask file if you forget to include it." << endl;
		return EXIT_SUCCESS;
	}

	if (ext_vol.compare("nhdr")) {
		if (ext_vol.compare("nrrd")) {
			cout << "NDHR or NRRD file formats only! Please, check file type." << endl;
			return EXIT_FAILURE;
		}
	}

	// Get image
	reader->SetFileName(inputMask);
	try
	{
		reader->Update();
		image = reader->GetOutput();
	}
	catch (itk::ExceptionObject)
	{
		cerr << "Can't read input mask file! Please, check that file is not corrupted." << endl;
		return EXIT_FAILURE;
	}
	cout << "Mask file loaded" << endl;
	return EXIT_SUCCESS;
}

template<typename D>
int DATA_SOURCE::save_to_file(const string& fname, typename D::Pointer& image, bool is_compress) {
	typedef itk::ImageFileWriter<D> ImageWriterType;
	ImageWriterType::Pointer writer = ImageWriterType::New();

	writer->SetFileName(fname);
	writer->SetInput(image);
	writer->SetUseCompression(is_compress);

	try
	{
		writer->Update();
	}
	catch (itk::ExceptionObject & err)
	{
		cerr << "Error while saving file on disk:\n " << err << endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void DATA_SOURCE::copy_header(DiffusionImagePointer& src, DiffusionImagePointer& dest) {
	//dest->SetMetaDataDictionary(src->GetMetaDataDictionary());
	dest->SetSpacing(src->GetSpacing());
	dest->SetDirection(src->GetDirection());
	dest->SetOrigin(src->GetOrigin());
	dest->SetRegions(src->GetLargestPossibleRegion());
	dest->SetNumberOfComponentsPerPixel(src->GetNumberOfComponentsPerPixel());
}

bool DATA_SOURCE::is_path_exists(const string &s)
{
	struct stat buffer;
	return (stat(s.c_str(), &buffer) == 0);
}

void DATA_SOURCE::testFNC() {
	//MatrixType test(19, 1);
	//test << -0.9, -0.8, -0.7, -0.6, -0.5, -0.4, -0.3, -0.2, -0.1, 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9;
	//MatrixType res = UM.polyleg(test, 5);
	SPH_RIDG test;
	//cout << test.M0;
	UtilMath UM = UtilMath();
	MatrixType g;
	UM.spiralsample(g, 2, 100);
	MatrixType A = test.RBasis(g);
	MatrixType normA = test.normBasis(A);

	test.normBasis(normA);
}

void DATA_SOURCE::readTestData(MatrixType& g, MatrixType& s) {
	fileToMatrix("D:\\test\\bvec.txt", g);
	fileToMatrix("D:\\test\\sign.txt", s);

	//normalize
	MatrixType gnorm = g.array().pow(2).rowwise().sum().sqrt();
	for (int i = 0; i < 51; ++i)
		g.row(i) = g.row(i) / gnorm(i);
}

void DATA_SOURCE::DWI2Matrix(DiffusionImagePointer &img, MaskImagePointer &mask,
	MatrixType &signal, unsigned &nGradImgs, unsigned &nOfImgs)
{
	int N_of_voxels = 0;
	if (mask != nullptr) {
		MaskIterator m_it(mask, mask->GetRequestedRegion());

		m_it.SetDirection(0);
		m_it.GoToBegin();
		while (!m_it.IsAtEnd())
		{
			while (!m_it.IsAtEndOfLine())
			{
				if (m_it.Get() == 1) {
					N_of_voxels += 1;
				}
				++m_it;
			}
			m_it.NextLine();
		}
	}
	else {
		DiffusionImageType::SizeType sz = img->GetLargestPossibleRegion().GetSize();
		N_of_voxels = sz[0] * sz[1] * sz[2];
	}

	unsigned first_grad_image_index = nOfImgs - nGradImgs;
	signal = MatrixType::Zero(nGradImgs, N_of_voxels);
	DiffusionImageType::PixelType voxel_content;

	// Iterate over all voxels
	if (mask != nullptr) {
		ConstIterator it_i(img, img->GetRequestedRegion());
		MaskIterator it_m(mask, mask->GetRequestedRegion());
		unsigned vox = 0;

		it_i.SetDirection(0);
		it_i.GoToBegin();
		it_m.SetDirection(0);
		it_m.GoToBegin();

		while (!it_i.IsAtEnd())
		{
			while (!it_i.IsAtEndOfLine())
			{
				if (it_m.Get() == 1) {
					voxel_content = it_i.Get();
					for (unsigned i = first_grad_image_index; i < nOfImgs; ++i)
						signal(i - first_grad_image_index, vox) = voxel_content.GetElement(i);
					++vox;
				}
				++it_i;
				++it_m;
			}
			it_i.NextLine();
			it_m.NextLine();
		}
	}
	else {
		ConstIterator it(img, img->GetRequestedRegion());
		unsigned vox = 0;
		it.SetDirection(0);
		it.GoToBegin();
		while (!it.IsAtEnd())
		{
			while (!it.IsAtEndOfLine())
			{
				voxel_content = it.Get();
				for (unsigned i = first_grad_image_index; i < nOfImgs; ++i)
					signal(i - first_grad_image_index, vox) = voxel_content.GetElement(i);

				++vox;
				++it;
			}
			it.NextLine();
		}
	}
}

void DATA_SOURCE::Matrix2DWI(DiffusionImagePointer &img, MaskImagePointer &mask, MatrixType &arr) {
	unsigned n_of_components = arr.rows();

	VariableVectorType vec_to_fill;
	vec_to_fill.SetSize(n_of_components);

	Iterator it(img, img->GetRequestedRegion());

	it.SetDirection(0);
	it.GoToBegin();
	unsigned vox = 0;

	if (mask != nullptr) {
		// Make a vector of zeros
		VariableVectorType zeros_vec;
		zeros_vec.SetSize(n_of_components);
		for (unsigned i = 0; i < n_of_components; ++i) {
			zeros_vec[i] = 0;
		}

		// Mask iterator
		MaskIterator it_m(mask, mask->GetRequestedRegion());

		it_m.SetDirection(0);
		it_m.GoToBegin();

		while (!it.IsAtEnd())
		{
			while (!it.IsAtEndOfLine())
			{
				if (it_m.Get() == 1) {
					for (unsigned i = 0; i < n_of_components; ++i) {
						vec_to_fill[i] = arr(i, vox);
					}
					it.Set(vec_to_fill);
				}
				else {
					it.Set(zeros_vec);
				}
			}
			++it;
			++it_m;
		}
		it.NextLine();
		it_m.NextLine();
	}
	else {
		while (!it.IsAtEnd())
		{
			while (!it.IsAtEndOfLine())
			{
				for (unsigned i = 0; i < n_of_components; ++i) {
					vec_to_fill[i] = arr(i, vox);
				}

				it.Set(vec_to_fill);
				++it;
				++vox;
			}
			it.NextLine();
		}
	}
}

void DATA_SOURCE::matrixToFile(const string& fname, MatrixType& matrix) {
	/*
	Write space separated matrix of MatrixType to text type file. Useful to debugging.
	*/
	ofstream file(fname);
	if (file.is_open())
		file << matrix << '\n';
}

void DATA_SOURCE::fileToMatrix(const string& fname, MatrixType& matrix)
{
	/*
	Read space separated matricies and vectors. Useful to debug with matlab code.
	*/
	unsigned number_of_rows = 0;
	unsigned number_of_cols = 0;
	std::string line;

	ifstream infile;
	infile.open(fname);

	// Get number of lines
	while (getline(infile, line))
		++number_of_rows;

	// Get number of columns
	infile.clear();
	infile.seekg(0, ios::beg);
	getline(infile, line);
	stringstream stream(line);
	while (stream)
	{
		std::string c;
		stream >> c;
		if (c.length())
			++number_of_cols;
	}

	matrix.resize(number_of_rows, number_of_cols);

	// Filling matrix
	infile.clear();
	infile.seekg(0, ios::beg);

	for (unsigned i = 0; i < number_of_rows; ++i)
	{
		getline(infile, line);
		stringstream stream(line);

		for (unsigned j = 0; j < number_of_cols; ++j)
			stream >> matrix(i, j);
	}
}

template <typename T>
void DATA_SOURCE::printVec(const string& name, vector<T>& v) {
	cout << name << endl;
	for (auto i : v)
		std::cout << i << ' ';
	cout << endl;
}

// Necessary templates for utilized types. Needed because implementation and declaration of function in separate files
template void DATA_SOURCE::printVec<int>(const string&, vector<int>&);
template void DATA_SOURCE::printVec<unsigned>(const string&, vector<unsigned>&);
template void DATA_SOURCE::printVec<Eigen::Index>(const string&, vector<Eigen::Index>&);

template int DATA_SOURCE::save_to_file<DiffusionImageType>(const string&, DiffusionImageType::Pointer&, bool);