#include <shlobj.h>
#include <iostream>
#include <fstream>
#include <string>
#include "sqlite3.h"

const char* banner() {
	const char* banner =
		"      __________________________________\n"
		"     | Sticky Situation V. 1.0       [x]|\n"
		"     |__________________________________|\n"
		"     | 			                |\n"
		"     | . -h for help menu               |\n"
		"     |			                |\n"
		"     | . Creator: helich0pper           |\n"
		"     |			                |\n"
		"     | . https://github.com/helich0pper |\n"
		"     |			                |\n"
		"     |			                |\n"
		"     |			                |\n"
		"     |			                |\n"
		"     |			                |\n"
		"     |__________________________________|\n"

		;

	return banner;
}

static void helpMenu() {
	puts("\nSticky Situation");
	puts("Usage:\n\t StickySituation.exe [OPTIONS]");
	puts("Options:\n\t-h Help menu");
	puts("\t-f <plum.sqlite> Manually chose file");
	puts("\t-o <output_file_name> Output raw data to file");
	puts("\t-O Output the contents with the same format as the note. Eg. Bold/Italic/Underlined");
}

struct Options {
	const char* inFile;
	const char* outFile;
	bool manualInput;
	bool formatOutput;
	bool inputDefault;
	bool outputDefault;
};

static Options* initOptions() {
	Options* options = new Options;

	options->inFile = "";
	options->outFile = "";
	options->manualInput = false;
	options->formatOutput = false;
	options->inputDefault = true;
	options->outputDefault = true;

	return options;
}

static void printFile(const char* filename, std::string out) {
	std::ofstream outFile(filename, std::ios_base::app);
	if (outFile.is_open()) {
		outFile << "Note:" << std::endl;
		outFile << out;
		std::cout << "Saved to " << filename << "\n\n";
	}
	else
		puts("Could not write to output file");
}

static std::string format(std::string line) {
	std::string ret = line.replace(0, 40, "");
	bool tagOpened = false;
	for (int i = 0; i < line.size(); i++) {
		if (line[i] == '\\') {
			if (line[i + 1] == 'b' && line[i + 2] != '0' && !tagOpened) {
				line.replace(i, 2, "");
				ret = line.insert(i, "<b>");
				tagOpened = true;
				continue;
			}
			if (line[i + 1] == 'b' && line[i + 2] == '0' && tagOpened) {
				line.replace(i, 3, "");
				ret = line.insert(i, "</b>");
				tagOpened = false;
				continue;
			}
			if (line[i + 1] == 'u' && line[i + 2] != '0' && !tagOpened) {
				line.replace(i, 2, "");
				ret = line.insert(i, "<u>");
				tagOpened = true;
				continue;
			}
			if (line[i + 1] == 'u' && line[i + 2] == '0' && tagOpened) {
				line.replace(i, 3, "");
				ret = line.insert(i, "</u>");
				tagOpened = false;
				continue;
			}
			if (line[i + 1] == 'i' && line[i + 2] != '0' && !tagOpened) {
				line.replace(i, 2, "");
				ret = line.insert(i, "<i>");
				tagOpened = true;
				continue;
			}
			if (line[i + 1] == 'i' && line[i + 2] == '0' && tagOpened) {
				line.replace(i, 3, "");
				ret = line.insert(i, "</i>");
				tagOpened = false;
			}
		}
	}

	return ret;
}

static void printFileFormated(const char* outFile) {
	std::string outHtml = outFile;
	outHtml += ".html";
	std::ifstream outFileOriginal(outFile);
	std::ofstream outFileFormatted(outHtml);
	std::string line = "";
	puts("Formatting...");
	if (outFileOriginal.is_open()) {
		outFileFormatted << "<html style='background-color:#292b2c'><div style='color:#ffffff'>";
		while (std::getline(outFileOriginal, line)) {
			line = format(line);
			outFileFormatted << line << "<br>";
		}
		outFileFormatted << "</div></html>";
		outFileOriginal.close();
		outFileFormatted.close();
		std::cout << "Saved formatted version to " << outHtml << std::endl;
	}
	else
		puts("Could not write formatted version to output file");
}

static int callback(void* filename, int argc, char** argv, char** azColName) {
	std::string out = "";

	for (int i = 0; i < argc; i++) {
		out += argv[i];
		out += '\n';
	}

	std::cout << "Note found!\n" << out << std::endl;
	if (filename != "")
		printFile((const char*)filename, out);

	return 0;
}

static void queryDB(const char* DB_PATH, Options* options) {
	sqlite3* db;
	const char* outFilename = static_cast<const char* const>(options->outFile);
	const char* query = "SELECT text FROM Note";
	if (sqlite3_open_v2(DB_PATH, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
		puts("\nCould not find data");
	}
	else {
		sqlite3_exec(db, query, callback, (void*)outFilename, NULL);
		sqlite3_close(db);
	}
}

static std::string getPath() {
	wchar_t* appdataPath;
	SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &appdataPath);
	std::wstring ws(appdataPath);
	std::string path(ws.begin(), ws.end());
	path += "\\Packages\\Microsoft.MicrosoftStickyNotes_8wekyb3d8bbwe\\LocalState\\plum.sqlite";

	return path;
}

bool checkArg(char** begin, char** end, const std::string& option) {
	return std::find(begin, end, option) != end;
}

const char* checkArgWithOption(char** begin, char** end, const std::string& option) {
	char** itr = std::find(begin, end, option);
	const char* ret = "";
	if (itr != end && ++itr != end) {
		ret = *itr;
	}

	return ret;
}

static bool configArgs(char** argv, int argc, Options* options) {
	bool ret = false;
	const char* inFile = checkArgWithOption(argv, argv + argc, "-f");
	const char* outFile = checkArgWithOption(argv, argv + argc, "-o");


	if (checkArg(argv, argv + argc, "-h")) {
		helpMenu();
		ret = true;
	}

	if (checkArg(argv, argv + argc, "-O")) {
		options->formatOutput = true;
	}

	if (inFile != "") {
		options->inFile = inFile;
		options->inputDefault = false;
	}

	if (outFile != "") {
		options->outFile = outFile;
		options->outputDefault = false;
	}

	return ret;
}

int main(int argc, char* argv[]) {
	Options* options = initOptions();
	std::string DB_PATH = "";
	bool err = false;

	std::cout << banner();

	if (argc != 1) {
		err = configArgs(argv, argc, options);
	}

	if (!err) {
		if (options->inputDefault) {
			puts("\nNo input provided, searching default location...\n");
			DB_PATH = getPath();
		}
		else {
			DB_PATH = options->inFile;
		}

		queryDB(DB_PATH.c_str(), options);

		if (options->formatOutput) {
			printFileFormated(options->outFile);
		}
	}

	return 0;
}
