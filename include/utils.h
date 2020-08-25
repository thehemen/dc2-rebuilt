#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <codecvt>
#include <regex>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <stdio.h>
#include <experimental/filesystem>

#ifndef UTILS_H
#define UTILS_H

using namespace std;
namespace fs = std::experimental::filesystem;

wstring read_file(const char* filename)
{
	wifstream ifile(filename);
    ifile.imbue(locale(ifile.getloc(), new codecvt_utf8<wchar_t>));
    wstringstream istream;
    istream << ifile.rdbuf();
    ifile.close();
    return istream.str();
}

string get_full_path(string directory, string filename)
{
	ostringstream out;
	out << directory << "/" << filename;
	return out.str();
}

void save_file(const char* filename, string str)
{
	ofstream ofile(filename);
	ofile << str;
	ofile.close();
}

int remove_file(const char* filename)
{
	return remove(filename);
}

time_t get_time_now()
{
	return chrono::system_clock::to_time_t(chrono::system_clock::now());
}

vector<size_t> find_all(wstring wstr, wstring wsubstr)
{
	vector<size_t> indexes;
	size_t index = 0;

	while((index = wstr.find(wsubstr, index)) != string::npos)
	{
		indexes.push_back(index);
		index++;
	}

	return indexes;
}

wstring get_substr(wstring wstr, size_t begin_index, size_t end_index)
{
	return wstr.substr(begin_index, end_index - begin_index + 1);
}

wstring replace_with(wstring wstr, wstring wsubstr_first, wstring wsubstr_second)
{
	return regex_replace(wstr, wregex(wsubstr_first), wsubstr_second);
}

time_t extract_datetime(wstring datetime_raw)
{
	string datetime_str(datetime_raw.begin(), datetime_raw.end());
    istringstream istream(datetime_str);
    struct tm tm;
    istream >> get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return mktime(&tm);
}

vector<size_t> get_all_occurences(wstring wstr, wchar_t sign)
{
	vector<size_t> positions;
	size_t pos = wstr.find(sign, 0);

	while(pos != wstring::npos)
	{
	    positions.push_back(pos);
	    pos = wstr.find(sign, pos + 1);
	}

	return positions;
}

wstring utf8_to_wstring(const string& str)
{
    wstring_convert<codecvt_utf8<wchar_t>> myconv;
    return myconv.from_bytes(str);
}

string wstring_to_utf8(const wstring& str)
{
    wstring_convert<codecvt_utf8<wchar_t>> myconv;
    return myconv.to_bytes(str);
}

vector<string> get_filename_list(string source_dir)
{
	vector<string> filenames;

	for(const auto & p: fs::recursive_directory_iterator(source_dir))
	{
		filenames.push_back(p.path());
	}

	return filenames;
}

string get_filename_only(string filename)
{
	return filename.substr(filename.find_last_of("/\\") + 1);
}

template <typename T>
int get_index_by_value(vector<T> vals, T val)
{ 
	auto pos = find(vals.begin(), vals.end(), val);

	if(pos != vals.end())
	{
		return distance(vals.begin(), pos);
	}
	else
	{
		return -1;
	}
} 

template <typename T>
pair<T, int> get_pair_by_max_value(map<T, int> counter)
{
	T best_value;
	int max_count = 0;

	for(const auto & [val, count] : counter)
	{
		if(count > max_count)
		{
			best_value = val;
			max_count = count;
		}
	}

	return pair<T, int>(best_value, max_count);
}

template <typename T>
bool remove_if_exists(vector<T>& vec, T elem)
{
    auto pos = find(vec.begin(), vec.end(), elem);

	if (pos != vec.end())
	{
	    vec.erase(pos);
	    return true;
	}
	else
	{
		return false;
	}
}

template <typename T1, typename T2>
bool remove_map_elem_if_exists(map<T1, T2>& my_map, T1 elem)
{
    auto pos = my_map.find(elem);

	if (pos != my_map.end())
	{
	    my_map.erase(pos);
	    return true;
	}
	else
	{
		return false;
	}
}

#endif