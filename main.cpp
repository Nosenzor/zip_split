
// ==========================================================================
// Copyright Romain NOSENZO(c) 2023. All Rights Reserved
// ==========================================================================
//

#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>

#include "Poco/Path.h"
#include "Poco/Zip/Compress.h"
#include "Poco/Zip/Decompress.h"
#include "Poco/Zip/ZipArchive.h"


#include <archive_reader.hpp>


#include <fmt/format.h>
#include <fmt/core.h>
#include <fmt/compile.h>
#include <fmt/std.h>

#include "boost/program_options.hpp"

#include <filesystem>
#include <fstream>
#include <atomic>
#include <iostream>
#include <algorithm>



namespace po = boost::program_options;
namespace fs = std::filesystem;


constexpr auto SubdirSize = 1000U;
std::vector<std::filesystem::path> PopulateFiles(const std::filesystem::path& iDir)
{
    std::vector<std::filesystem::path> AllInFiles;
    for (const fs::directory_entry& dir_entry:
            fs::recursive_directory_iterator(iDir))
    {
        if (dir_entry.path().filename().string().front()!='.' &&
            fs::is_regular_file(dir_entry.path()))
        {
            AllInFiles.push_back(dir_entry.path());
        }
    }
    return AllInFiles;
}


std::vector<std::filesystem::path>  UnzipFile(const std::filesystem::path& inputpath,
                                               const std::filesystem::path& outputpath)
{

    if(inputpath.extension().string()==".7z") {
        //fmt::print("Unzipping 7z using moor {} into {}\n", inputpath, outputpath);
        moor::ArchiveReader reader1(inputpath.string());
//        auto do_more = reader1.ExtractNext(outputpath.string());
//        while (do_more)
//        {
//            do_more = reader1.ExtractNext(outputpath.string());
//        }
        auto data = reader1.ExtractNext();
        std::vector<std::filesystem::path> Outlist;
        while(data.first.length() > 0)
        {
            //fmt::print("data.first {} data.second {}",data.first, data.second.size());
            fs::path outfname=outputpath / data.first;
            Outlist.push_back(outfname);
            fs::create_directories(outfname.parent_path());
            //fmt::print("Write {} octets into {} \n",data.second.size(),outfname);

            std::ofstream output(outfname, std::ios::binary);
            output.write(reinterpret_cast<char*>(data.second.data()),data.second.size());
            output.close();
            data = reader1.ExtractNext();

        }
        return Outlist;
    }
    else
    {
        //fmt::print("Unzipping using poco {} into {}\n",inputpath,outputpath);
        std::ifstream inp(inputpath, std::ios::binary);
        Poco::Path Output(std::filesystem::absolute(outputpath).string(), Poco::Path::PATH_NATIVE);
        Poco::Zip::Decompress dec(inp, Output);
        std::string Filename;
        auto ZipInfo = dec.decompressAllFiles();
        //fmt::print("Unzip finish\n");
        std::vector<std::filesystem::path> Outlist(std::distance(ZipInfo.fileInfoBegin(),
                                                                 ZipInfo.fileInfoEnd()));
        std::transform(ZipInfo.fileInfoBegin(), ZipInfo.fileInfoEnd(),
                       Outlist.begin(),
                       [&outputpath](const std::pair<std::string, Poco::Zip::ZipFileInfo>& file1) -> std::filesystem::path
                       {
                           return outputpath / std::filesystem::path(file1.second.getFileName());
                       });
        //fmt::print("Unzipped {} files {}\n",outputpath,Outlist.size());
        return Outlist;
    }

}

void ZipFile(const std::filesystem::path& inputpath,
          const std::filesystem::path& outputpath)
{
    Poco::Path FileToZip(inputpath);
    if(FileToZip.isFile())
    {
        std::ofstream output(outputpath, std::ios::binary);
        //Poco::Path Output(std::filesystem::absolute(outputpath).string(), Poco::Path::PATH_NATIVE);
        Poco::Zip::Compress Compressor(output, true);
        Compressor.addFile(FileToZip, FileToZip.getFileName());
        Compressor.close(); // MUST be done to finalize the Zip file
        output.close();
    }
}

// /Users/Romain/CLionProjects/PhotoTools/cmake-build-release/bin/PB2CLI --i=/Volumes/Projets/Thingi10K/raw_meshes --o=/Volumes/Projets/Output
int main(int argc, const char** argv)
{

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")
    ("input_dir,i", po::value<std::filesystem::path>(), "input directory")
    ("output_dir,o", po::value<std::filesystem::path>(), "output directory");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return 1;
    }

    std::filesystem::path InputDir;
    std::vector<std::filesystem::path> AllInFiles;
    if (vm.count("input_dir"))
    {
        InputDir = vm["input_dir"].as<std::filesystem::path>();
        fmt::print("inputdir {}\n", InputDir.string());
        if (fs::exists(InputDir))
        {
            AllInFiles = PopulateFiles(InputDir);
        }
        else
        {
            fmt::print("/!\\ CANNOT FIND inputdir {}\n", InputDir.string());
        }
        fmt::print("Found {} files \n", AllInFiles.size());
    }

    std::filesystem::path OutputDir;
    if (vm.count("output_dir"))
    {
        OutputDir = vm["output_dir"].as<std::filesystem::path>();

        auto now = std::chrono::system_clock::now();
        auto now_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream Date;
        Date << std::put_time(std::localtime(&now_t), "%Y-%m-%d-%H-%M-%S");
        OutputDir /= Date.str();
        bool CreateDir = fs::create_directories(OutputDir);
        if (CreateDir)
        {
            fmt::print("output_dir {} Succesfully created\n", OutputDir.string());
        }
        else
        {
            fmt::print("output_dir {}\n", OutputDir.string());
        }
    }


    const auto NbFiles = AllInFiles.size();

    std::atomic<size_t> NbInZipFilesDone{0};
    std::atomic<size_t> NbOutZipFilesDone{0};

    auto fUnZipAndReZip=[&NbInZipFilesDone, &NbOutZipFilesDone,&OutputDir,NbFiles]
            (const auto & filename) {

        ++NbInZipFilesDone;
        fmt::print("Processing file {} : {}/{}\n",filename,NbInZipFilesDone,NbFiles);
        std::string TmpDirName = fmt::format("Temp{}", NbInZipFilesDone.load());
        fs::create_directories(OutputDir / TmpDirName);
        auto AllFilesInArchive = UnzipFile(filename, OutputDir / TmpDirName);


        tbb::parallel_for_each(AllFilesInArchive.begin(), AllFilesInArchive.end(),
                               [&NbOutZipFilesDone, &OutputDir]
                                       (const auto &file_to_zip) {
                                   if (fs::is_regular_file(file_to_zip)) {

                                       std::string SubDirName = fmt::format("S{}k",
                                                                            NbOutZipFilesDone.load() / SubdirSize);
                                       fs::create_directories(OutputDir / SubDirName);
                                       fs::path ZipFilePath = OutputDir / SubDirName /
                                                              (file_to_zip.stem().string() + ".zip");
                                       ++NbOutZipFilesDone;
                                       ZipFile(file_to_zip, ZipFilePath);
                                   }
                               });
        // Cleaning
        fs::remove_all(OutputDir / TmpDirName);
        fmt::print("You Can remove {}\n", filename);
        //fs::remove_all(filename);


        //barbar.set_progress(100 * ((NbInZipFilesDone + 1) / NbFiles));
    };
    bool OuterLoopSequential=true;
    if(OuterLoopSequential)
    {
        std::for_each(AllInFiles.begin(), AllInFiles.end(), fUnZipAndReZip);
    }
    else
    {
        tbb::parallel_for_each(AllInFiles.begin(), AllInFiles.end(), fUnZipAndReZip);
    }
    //barbar.mark_as_completed();

    // Show cursor
    //pb::show_console_cursor(true);
    fmt::print("Work DONE !\n");
    return 0;
}