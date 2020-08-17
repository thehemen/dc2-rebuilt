#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <codecvt>
#include <regex>
#include <iomanip>
#include <algorithm>
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

int get_index_by_string(vector<string> str_list, string str) 
{ 
	return distance(str_list.begin(), find(str_list.begin(), str_list.end(), str));
} 

pair<string, int> get_pair_by_max_value(map<string, int> counter)
{
	string category_best("other");
	int max_count = 0;

	for(const auto & [category, count] : counter)
	{
		if(count > max_count)
		{
			max_count = count;
			category_best = category;
		}
	}

	return pair<string, int>(category_best, max_count);
}

#endif