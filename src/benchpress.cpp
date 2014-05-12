//============================================================================
// Name        : benchpress.cpp
// Author      : Ulrik Kofoed Pedersen
// Version     :
// Copyright   : MIT. See LICENSE file.
// Description : Use blosc with different algorithms to compress data from
//               datasets in an HDF5 file - and benchmark the performance
//============================================================================

#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
using namespace std;

#include <blosc.h>
#include <hdf5.h>
#include <hdf5_hl.h>

#include <boost/program_options.hpp>
namespace po = boost::program_options;


class Image {
public:
    Image() : pdata(NULL), bytes(0){};
    Image(unsigned int width, unsigned int height, int bpp, void* pdata);
    Image(unsigned int nbytes, void* pdata);
    Image(const Image& src);     // copy constructor
    void next(const Image& src); // Increment pointer to next image after src
    void next();                 // Increment pointer to next image position
    // Operators
    Image& operator=( const Image& src );
private:
    void * pdata;
    unsigned int bytes;
};

void * read_dataset(const string& file_name,
        const string& dataset_name, vector<Image>& images);

void * read_dataset(const string& file_name,
        const string& dataset_name, vector<Image>& images)
{
    herr_t herr;
    int rank=0;
    H5T_class_t class_id;
    size_t type_size;
    hid_t file;
    hid_t dtype;

    file = H5Fopen(file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    herr = H5LTget_dataset_ndims(file, dataset_name.c_str(), &rank);
    hsize_t *dims = (hsize_t*)calloc(rank, sizeof(hsize_t));

    herr = H5LTget_dataset_info(file, dataset_name.c_str(), dims, &class_id, &type_size);

    unsigned int npixels = 1;
    for (int i=0;i<rank; i++)
    {
        npixels *= dims[i];
    }

    void * pdata = calloc(npixels, type_size);
    herr = H5LTread_dataset_int(file, dataset_name.c_str(), (int*)pdata);

    // Create a list of images and push it to the user supplied 'images' vector
    Image img(npixels * type_size, pdata);
    for (unsigned int i = 0; i<dims[0]; i++)
    {
        images.push_back(img);
        img.next();
    }

    free(dims);
    herr = H5Fclose(file);
    return pdata;
}


Image::Image(unsigned int width, unsigned int height, int bpp, void* pdata)
 : pdata(pdata)
{
    this->bytes = width * height * bpp;
}

Image::Image(unsigned int nbytes, void* pdata)
 : pdata(pdata), bytes(nbytes)
{
}

Image::Image(const Image& src)
{
    this->bytes = src.bytes;
    this->pdata = src.pdata;
}

void Image::next(const Image& src)
{
    this->pdata = (char*)(src.pdata) + src.bytes;
}

void Image::next()
{
    this->next(*this);
}

Image& Image::operator=( const Image& src )
{
    if (this != &src) {
        this->bytes = src.bytes;
        this->pdata = src.pdata;
    }
    return *this;
}



int main(int argc, char* argv[]) {
    int threads;
    int compress_level;
    po::options_description opt_desc("Available options");
    po::variables_map var_map;

    try {
    opt_desc.add_options()
            ("help,h", "Show online help")
            ("algorithm,a", po::value< string >()->default_value("blosclz"), "Compression algorithm")
            ("threads,t", po::value<int>(&threads)->default_value(1), "Number of threads to use")
            ("level,l", po::value<int>(&compress_level)->default_value(0), "Compression level [0..9]")
            ("file", po::value< string >()->default_value(""), "Input file")
            ("dataset", po::value<string>()->default_value(""), "Input dataset")
    ;

    po::positional_options_description p;
    p.add("file", 1);
    p.add("dataset", 1);


    po::store(po::command_line_parser(argc, argv).options(opt_desc).positional(p).run(), var_map);
    po::notify(var_map);

    if (var_map.count("help")) {
        cout << opt_desc << "\n" << endl;
        return 1;
    }

    cout << "Reading from file: " << var_map["file"].as<string>() << endl;
    cout << "Input dataset:     " << var_map["dataset"].as<string>() << endl;
    }
    catch (std::exception& e) // Catch option parsing exceptions
    {
        cout << "Option parsing exception: " << endl;
        cout << "\t boost error: \"" << e.what() << "\"" << endl;
        cout << "\n" << opt_desc << endl;
        return 1;
    }

    // Calculate data buffer sizes
    // Read data from file
    vector<Image> images;
    void * pdata = read_dataset(var_map["file"].as<string>(),
            var_map["dataset"].as<string>(), images);

    cout << "Number of images read: " << images.size() << endl;

    // Timer
    //    Compress data to memory buffer

    // Print report

    free (pdata);
    return 0;
}
