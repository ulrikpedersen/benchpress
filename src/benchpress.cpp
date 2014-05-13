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
#include <sstream>
using namespace std;

#include <blosc.h>
#include <hdf5.h>
#include <hdf5_hl.h>

#include <boost/program_options.hpp>
#include <boost/timer/timer.hpp>
namespace po = boost::program_options;
using boost::timer::cpu_timer;
using boost::timer::cpu_times;
using boost::timer::nanosecond_type;

class Image {
public:
    Image() : pdata(NULL), bytes(0), typesize(0){};
    Image(unsigned int width, unsigned int height, int bpp, void* pdata);
    Image(unsigned int nbytes, void* pdata);
    Image(const Image& src);     // copy constructor
    void next(const Image& src); // Increment pointer to next image after src
    void next();                 // Increment pointer to next image position
    // Operators
    Image& operator=( const Image& src );
    const void* data_ptr();
    unsigned int frame_bytes() {return this->bytes;}
    size_t get_typesize(){return this->typesize;}
private:
    void * pdata;
    unsigned int bytes;
    size_t typesize;
};

void * read_dataset(const string& file_name,
        const string& dataset_name, vector<Image>& images);
vector<string> &split_string(const string &s, char delim, vector<string> &elems);

vector<string> &split_string(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

void * read_dataset(const string& file_name,
        const string& dataset_name, vector<Image>& images)
{
    herr_t herr;
    int rank=0;
    H5T_class_t class_id;
    size_t type_size;
    hid_t file;

    file = H5Fopen(file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    herr = H5LTget_dataset_ndims(file, dataset_name.c_str(), &rank);
    hsize_t *dims = (hsize_t*)calloc(rank, sizeof(hsize_t));

    herr = H5LTget_dataset_info(file, dataset_name.c_str(), dims, &class_id, &type_size);

    unsigned int dset_pixels = 1;
    for (int i=0;i<rank; i++) dset_pixels *= dims[i];

    void * pdata = calloc(dset_pixels, type_size);
    herr = H5LTread_dataset_int(file, dataset_name.c_str(), (int*)pdata);

    // Create a list of images and push it to the user supplied 'images' vector
    unsigned int frame_pixels = 1;
    for (int i = 1; i<rank; i++) frame_pixels *= dims[i];
    Image img(frame_pixels * type_size, pdata);
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
 : pdata(pdata), typesize(bpp)
{
    this->bytes = width * height * bpp;
}

Image::Image(unsigned int nbytes, void* pdata)
 : pdata(pdata), bytes(nbytes), typesize(4)
{
}

Image::Image(const Image& src)
{
    this->bytes = src.bytes;
    this->pdata = src.pdata;
    this->typesize = src.typesize;
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
        this->typesize = src.typesize;
    }
    return *this;
}

const void* Image::data_ptr()
{
    return this->pdata;
}


int main(int argc, char* argv[]) {
    int threads;
    int compress_level;
    int shuffle;
    bool verbose=false;
    int iterations;
    string algorithm;
    po::options_description opt_desc("Available options");
    po::variables_map var_map;
    const char * compression_algorithms = blosc_list_compressors();

    // Handle CLI options and arguments
    try {
    opt_desc.add_options()
            ("help,h", "Show online help")
            ("algorithm,a", po::value< string >(&algorithm)->default_value("blosclz"), compression_algorithms)
            ("threads,t", po::value<int>(&threads)->default_value(1), "Number of threads to use")
            ("level,l", po::value<int>(&compress_level)->default_value(0), "Compression level [0..9]")
            ("shuffle,s", po::value<int>(&shuffle)->default_value(1), "Precondition shuffle")
            ("iterations,i", po::value<int>(&iterations)->default_value(1), "Number of iterations over input dataset")
            ("list", "List available compression algorithms")
            ("verbose,v", "Print lots of timestamps")
            ("file", po::value< string >()->default_value(""), "Input file")
            ("dataset", po::value<string>()->default_value(""), "Input dataset")
    ;

    po::positional_options_description p;
    p.add("file", 1);     // Input file name
    p.add("dataset", 1);  // Input dataset (inside file)

    po::store(po::command_line_parser(argc, argv).options(opt_desc).positional(p).run(), var_map);
    po::notify(var_map);

    if (var_map.count("verbose")) verbose = true;
    if (var_map.count("help")) {
        cout << opt_desc << "\n" << endl;
        return 1;
    }

    if (var_map.count("list")) {
        vector<string> algo_list;
        split_string(string (compression_algorithms), ',', algo_list);
        cout << "Available compression algorithms:" << endl;
        vector<string>::iterator it;
        for (it = algo_list.begin(); it != algo_list.end(); ++it)
        {
            char * complib;
            char * complib_version;
            blosc_get_complib_info(const_cast<char*>( it->c_str() ), &complib, &complib_version);
            cout << "\t" << it->c_str() << "\t" << complib_version << "\t" << complib << endl;
            free(complib); free(complib_version);
        }
        return -1;
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

    // Read the input images from the file/dataset and push
    // into the images vector
    vector<Image> images;
    void * pdata = read_dataset(var_map["file"].as<string>(),
            var_map["dataset"].as<string>(), images);

    cout << "Number of images read: " << images.size() << endl;

    // Initialise blosc and configure number of threads and algorithm
    blosc_init();
    blosc_set_nthreads(threads);
    blosc_set_compressor(algorithm.c_str());
    size_t dest_size = images[0].frame_bytes() + BLOSC_MAX_OVERHEAD;
    void * dest_buf = calloc(dest_size, sizeof(char));

    // Loop through the vector of frames, compress each one and time the loop.
    // The compressed data is not used - but the buffer is reused for next iteration
    vector<cpu_times> elapsed_times;
    cpu_timer timer;

    vector<Image>::iterator it;
    unsigned long total_cbytes = 0;

    for (int i = 0; i<iterations; i++)
    {
        if (verbose) cout << "Iteration=" << i << endl;
        for (it = images.begin(); it != images.end(); ++it)
        {
            int cbytes = blosc_compress(compress_level, shuffle,
                    it->get_typesize(),it->frame_bytes(), it->data_ptr(),
                    dest_buf, dest_size);
            total_cbytes += cbytes;
            if (verbose)
            {
                double ratio = (double)cbytes/(double)(it->frame_bytes());
                cout << "Ratio: " << 1./ratio << " (" << cbytes << "/" << it->frame_bytes() << ")";
                cpu_times wus_times = timer.elapsed();
                elapsed_times.push_back(wus_times);
                cout << " Wall: "   << wus_times.wall/1000000000.;
                cout << " User: "   << wus_times.user/1000000000.;
                cout << " System: " << wus_times.system/1000000000. << endl;
            }
        }
    }
    timer.stop();
    cpu_times wus_final = timer.elapsed();

    double dset_megabyte = (iterations * images.size() * images[0].frame_bytes())/(1024.*1024.);
    double comp_megabyte = total_cbytes/(1024. * 1024.);
    double ratio = dset_megabyte/comp_megabyte;
    double data_rate = dset_megabyte/(wus_final.wall/1000000000.);

    cout << " Dataset=" << dset_megabyte << "MB\tCompressed=" << comp_megabyte << "MB" << endl;
    cout << "Time: Wall=" << wus_final.wall/1000000000.;
    cout << "\tUser=" << wus_final.user/1000000000.;
    cout << "\tSystem=" << wus_final.system/1000000000. << endl;
    cout << "CONFIG:\talgo=" << algorithm << "\tlevel=" << compress_level << "\tthreads=" << threads << endl;
    cout << "RESULT:\tRatio=" << ratio << "\tDatarate=" << data_rate << " MB/s" << endl;

    blosc_destroy();
    free (pdata);
    free(dest_buf);
    return 0;
}
