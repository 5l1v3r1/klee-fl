
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>

#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>


#define CLANG_PATH "clang"
#define CLANGXX_PATH "clang++"
#define LLVM_AR_PATH "llvm-ar"
#define LLVM_COMPILER_PATH "/usr/bin/"


using namespace std;


typedef std::vector<std::string> path_list;
typedef std::map<std::string,std::string> md5_list;


int is_debug_output = 0;


const char* get_llvm_compiler_path(void) {
    char* llvm_compiler_path = getenv("LLVM_COMPILER_PATH");

    if (!llvm_compiler_path)
        return LLVM_COMPILER_PATH;

    return llvm_compiler_path;
}

std::string get_clang_path(void) {
    const char* clang_path = getenv("KFL_CLANG");

    if (!clang_path)
        clang_path = CLANG_PATH;

    const char* llvm_path = get_llvm_compiler_path();
    std::string result_path = llvm_path;

    result_path += "/";
    result_path += clang_path;

    if (!access(result_path.c_str(),F_OK)) {
        errno = 0;

        return result_path;
    }

    result_path = clang_path;

    return result_path;
}

std::string get_clangpp_path(void) {
    const char* clangpp_path = getenv("KFL_CLANGXX");

    if (!clangpp_path)
        clangpp_path = CLANGXX_PATH;

    const char* llvm_path = get_llvm_compiler_path();
    std::string result_path = llvm_path;

    result_path += "/";
    result_path += clangpp_path;

    if (!access(result_path.c_str(),F_OK)) {
        errno = 0;

        return result_path;
    }

    result_path = clangpp_path;

    return result_path;
}

std::string get_llvm_ar_path(void) {
    const char* llvm_ar_path = getenv("KFL_LLVM_AR");

    if (!llvm_ar_path)
        llvm_ar_path = LLVM_AR_PATH;

    const char* llvm_path = get_llvm_compiler_path();
    std::string result_path = llvm_path;

    result_path += "/";
    result_path += llvm_ar_path;

    if (!access(result_path.c_str(),F_OK)) {
        errno = 0;

        return result_path;
    }

    result_path = llvm_ar_path;

    return result_path;
}

std::string get_compiler_flags(void) {
    const char* compiler_flags = getenv("KFL_CFLAG");
    std::string result;

    if (!compiler_flags)
        return result;

    result = compiler_flags;

    return result;
}


std::string get_file_md5(const string& path) {
    std::string command = "md5 ";
    command += path;

    FILE* process_handle = popen(command.c_str(),"r");
    char md5_buffer[256] = {0};

    fread(&md5_buffer,1,sizeof(md5_buffer),process_handle);
    pclose(process_handle);

    std::string result = md5_buffer;
    unsigned int offset = result.find("=");

    if (-1 != offset) {
        result = result.substr(offset + 2);
        result = result.substr(0,result.length() - 1);

        return result;
    }

    return "";
}

void execute(const char* command) {
    FILE* process_handle = popen(command,"r");

    pclose(process_handle);
}

path_list* list_dir(const string& path) {
    struct dirent* file_handle = NULL;
    DIR* dir_handle = opendir(path.c_str());

    if (NULL == dir_handle)
        return NULL;

    path_list* result = new path_list();

    while (NULL != (file_handle = readdir(dir_handle))) {
        std::string file_name = file_handle->d_name;

        if (4 == file_handle->d_type) {
            if ("." == file_name || ".." == file_name)
                continue;

            path_list* subdir_file = list_dir(path + "/" + std::string(file_handle->d_name));

            if (NULL == subdir_file)
                continue;

            for (path_list::iterator subdir_file_iterator = subdir_file->begin();subdir_file_iterator != subdir_file->end();++subdir_file_iterator)
                result->push_back(*subdir_file_iterator);

            delete subdir_file;
        } else {
            if (is_debug_output) {
                std::string current_path = path + "/" + file_name;

                printf("file_handle->d_name = %s file_handle->d_type = %d \n",current_path.c_str(),file_handle->d_type);
            }

            if (-1 != file_name.find(".bc") && -1 == file_name.find(".bca"))
                result->push_back(path + "/" + std::string(file_handle->d_name));
        }
    }

    return result;
}

path_list* filter_similar(path_list* file_list) {
    path_list* result = new path_list;
    md5_list md5_record_list;

    for (path_list::iterator file_list_iterator = file_list->begin();file_list_iterator != file_list->end();++file_list_iterator) {
        std::string md5_hash = get_file_md5(*file_list_iterator);

        if (is_debug_output)
            printf("file_path = %s ,file_md5 = %s\n",file_list_iterator->c_str(),md5_hash.c_str());

        if (!md5_record_list.count(md5_hash)) {

            md5_record_list[md5_hash] = *file_list_iterator;

            result->push_back(*file_list_iterator);
        } else {
            if (is_debug_output)
                printf("Exist Similar File  file_path = %s (%s) ,file_md5 = %s\n",file_list_iterator->c_str(),md5_record_list[md5_hash].c_str(),md5_hash.c_str());
        }

    }

    return result;
}

unsigned int print_dir_file(path_list* file_list) {
    unsigned int file_count = 0;

    printf("print_dir_file() Output:\n");

    for (path_list::iterator file_list_iterator = file_list->begin();file_list_iterator != file_list->end();++file_list_iterator) {
        printf("-> %s \n",file_list_iterator->c_str());

        file_count += 1;
    }

    return file_count;
}

void print_parameters(char** parameter_list) {
    printf("Argument : ");

    for (int index = 0;NULL != parameter_list[index];++index)
        printf("%s ",parameter_list[index]);

    printf("\n");
}

void print_help(void) {
    printf("Using : [ KFL_CFLAG=\"-I.\" ] klee-build %%Fuzzer_Path%% %%Project_Build_Dir%% [ import_file1 import_file2 .. ]\n");
    //printf("  -bc : build fuzzer to LLVM BitCode\n");
    //printf("  -bf : build fuzzer for klee fuzzing\n");
    printf("  Fuzzer_Path : Custom Fuzzer Path\n");
    printf("  Project_Build_Dir : Project Path\n");
    printf("  import_file : Import Link File Path\n");
    printf("  KFL_CFLAG : Custom Compiler Flags\n");
    printf("Example : \n");
    printf("  ./klee-build ./test_code/test_fuzzing_entry.c ./test_code/\n");
    printf("  klee-build klee_fuzzer.c . libcares_la-ares_create_query.bc libcares_la-ares_library_init.bc\n");
    //printf("  ./klee-build -bc ./test_code/test_fuzzing_entry.c\n");
    //printf("  ./klee-build -bf ./test_code\n");
}

std::string compile_fuzzer_to_bitcode(const char* fuzzer_path) {
    std::string call_parameters;
    std::string compiler_flags = get_compiler_flags();
    std::string output_path = fuzzer_path;

    if (-1 != output_path.rfind(".c"))
        output_path = output_path.substr(0,output_path.rfind(".c")) + ".bc";
    else if (-1 != output_path.rfind(".cpp"))
        output_path = output_path.substr(0,output_path.rfind(".cpp")) + ".bc";

    call_parameters += get_clang_path();
    call_parameters += (char*)" -g ";
    call_parameters += (char*)" -emit-llvm ";
    call_parameters += (char*)" -c ";
    call_parameters += (char*)fuzzer_path;
    call_parameters += (char*)" -o ";
    call_parameters += (char*)output_path.c_str();
    call_parameters += (char*)" ";

    if (!compiler_flags.empty())
        call_parameters += compiler_flags;

    printf("Argument : %s\n",call_parameters.c_str());
    execute(call_parameters.c_str());
    printf("errno = %d\n",errno);

    if (errno)
        return "";

    return output_path;
}

void compile_fuzzer_to_lib(const char* project_path,path_list* llvm_bitcode_file_list,unsigned int file_count) {
    std::string call_parameters;
    std::string output_path = project_path;
    output_path += "/klee_fuzzer.link.bc";

    call_parameters += "llvm-link";//get_llvm_ar_path();
    call_parameters += " -o ";//(char*)" rsc ";
    call_parameters += (char*)output_path.c_str();
    call_parameters += (char*)" ";

    unsigned int file_index = 0;
    path_list* filter_list = filter_similar(llvm_bitcode_file_list);

    for (path_list::iterator file_list_iterator = filter_list->begin();file_list_iterator != filter_list->end();++file_list_iterator,++file_index) {
        call_parameters += (char*)file_list_iterator->c_str();
        call_parameters += " ";
    }

    printf("Argument : %s\n",call_parameters.c_str());

    if (!access(output_path.c_str(),F_OK))
        remove(output_path.c_str());
    else
        errno = 0;

    execute(call_parameters.c_str());

    delete filter_list;
}

int main(int argc,char** argv) {
    if (3 > argc) {
        print_help();

        return 1;
    }

    char* fuzzer_path = argv[1];

    printf("Ready to Compiler Fuzzer \n");

    std::string output_bitcode_path = compile_fuzzer_to_bitcode(fuzzer_path);

    if (output_bitcode_path.empty()) {
        printf("Compile Fuzzer to BitCode Error ! ..\n");

        return 1;
    }

    path_list* llvm_bitcode_file_list = new path_list;

    for (int import_path_index = 2;import_path_index < argc;++import_path_index) {
        char* import_path = argv[import_path_index];
        struct stat get_file_stat = {0};

        if (stat(import_path,&get_file_stat)) {
            printf("import_path = %s not Found ! ..\n",import_path);

            return 1;
        }

        if (get_file_stat.st_mode & S_IFDIR) {
            path_list* import_path_file_list = list_dir(std::string(import_path));

            for (path_list::iterator file_path  = import_path_file_list->begin();
                                     file_path != import_path_file_list->end();
                                     ++file_path)
                llvm_bitcode_file_list->push_back(*file_path);

            delete import_path_file_list;
        } else {
            llvm_bitcode_file_list->push_back(import_path);
        }
    }

    unsigned int file_count = print_dir_file(llvm_bitcode_file_list);

    printf("project .bc file count = %d \n",file_count);

    if (!file_count) {
        printf("import_path have not LLVM BitCode File ..\n");
        printf("Check it is you compile the project with klee-clang ?..");

        return 1;
    }

    printf("Ready to Compiler Lib \n");

    compile_fuzzer_to_lib(".",llvm_bitcode_file_list,file_count);

    printf("Compile All Success ..\n");

    delete llvm_bitcode_file_list;

    return 0;
}

