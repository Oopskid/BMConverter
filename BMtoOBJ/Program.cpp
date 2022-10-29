#include <iostream>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <fstream>

#include <winsock2.h>
#pragma comment(lib, "ws2_32")

/*
-BM = 


*/

void BMgetIndices(std::ifstream& f, char* buffer, std::vector<int>& indices, size_t indiceSize = 2)
{
	int indiceCount = -1;

	f.read(buffer, indiceSize * 2);
	indiceCount = *(short*)(buffer);
	std::cout << "There are " << indiceCount << " indices to be read \n";
	if (indiceCount % 3 != 0) { std::cout << "Warning: number of indices is not a multiple of three \n"; }

	//Indices
	for (int i = 0; f.good() && i < indiceCount; i++)
	{
		short indice;
		f.read(buffer, indiceSize);
		indice = *(short*)(buffer);
		indices.push_back(indice);

		std::cout << indice << std::endl;
	}
}

void BMgetAttributes(std::ifstream& f, char* buffer, std::vector<std::vector<float>>& vertAttribs, size_t attribCount = 8, size_t maxVerts = ~(0), size_t valSize = 4)
{
	vertAttribs.push_back(std::vector<float>());

	bool nullSwitch = false;
	while (f.good())
	{
		bool newRow = vertAttribs.back().size() >= attribCount;
		float attrib;

		if (newRow && vertAttribs.size() >= maxVerts) { break; }

		#define nextAttrib()\
		f.read(buffer, valSize);\
		attrib = *(float*)(buffer);\

		nextAttrib();
		if (isnan(attrib)) { continue; }
		if (attrib == NULL) { if (nullSwitch) { break; } }
		nullSwitch = attrib == NULL;

		//Attribute row
		if (newRow) { vertAttribs.push_back(std::vector<float>()); }
		vertAttribs.back().push_back(attrib);
	}

	std::cout << "There are " << vertAttribs.size() << " vertices! \n";
}

void BMfunc(int argc, char* argv[], bool skinnedSwitch = false)
{
	if (argc < 2) { std::cout << "BM call with insufficient args \n"; return; }

	std::ifstream f(argv[0], std::ios::binary);
	std::ofstream out(argv[1]);
	if (f.good() && out.good())
	{
		// BM format details
		static const size_t headerOffset = skinnedSwitch ? 16 : 8;
		static const size_t vertOffset = 8;
		static const size_t bufferSize = 8; //Max buffer size
		static const size_t indiceSize = 2; //USHORT
		static const size_t valSize = 4; //FLOAT
		
		static const size_t attribCount = skinnedSwitch ? 13 : 8;
		static const size_t xAttrib = 0;
		static const size_t yAttrib = 1;
		static const size_t zAttrib = 2;
		static const size_t n0Attrib = 3;
		static const size_t n1Attrib = 4;
		static const size_t n2Attrib = 5;
		static const size_t uAttrib = 6;
		static const size_t vAttrib = 7;
		//Skinned
		static const size_t w0Attrib = 8;
		static const size_t w1Attrib = 9;
		static const size_t w2Attrib = 10;
		static const size_t w3Attrib = 11;
		static const size_t w4Attrib = 12;

		char buffer[bufferSize];
		memset(buffer, NULL, bufferSize);

		//Data
		int maxVertexCount = ~0;

		//Skip header
		f.seekg(headerOffset, SEEK_CUR);
		
		std::vector<int> indices;
		std::vector<std::vector<float>> attribs;

		if (skinnedSwitch)
		{
			f.read(buffer, indiceSize * 2);
			maxVertexCount = *(short*)(buffer);

			//Attributes
			BMgetAttributes(f, buffer, attribs, attribCount, maxVertexCount, valSize);

			//Need to skip a value for some reason
			f.seekg(valSize, SEEK_CUR);

			//Indices
			BMgetIndices(f, buffer, indices, indiceSize);
		}
		else
		{
			//Indices
			BMgetIndices(f, buffer, indices, indiceSize);

			//Need to skip a value for some reason
			f.seekg(valSize, SEEK_CUR);

			//Attributes
			BMgetAttributes(f, buffer, attribs, attribCount, maxVertexCount, valSize);
		}

		// Write verts
		for (int i = 0; i < attribs.size(); i++)
		{
			if(attribs[i].size() >= n0Attrib)
				out << "v " << attribs[i][xAttrib] << " " << attribs[i][yAttrib] << " " << attribs[i][zAttrib] << std::endl;
		}

		out << std::endl;

		// Write UVs
		for (int i = 0; i < attribs.size(); i++)
		{
			if (attribs[i].size() > vAttrib)
				out << "vt " << attribs[i][uAttrib] << " " << (1.0f - attribs[i][vAttrib]) << " " << (float)0.0f << std::endl;
		}

		out << std::endl;

		// Its indices time!
		for (int i = 0; i+2 < indices.size(); i+=3)
		{
			int i1 = indices[i] + 1;
			int i2 = indices[i+1] + 1;
			int i3 = indices[i+2] + 1;

			out << "f " << i1 << "/" << i1 << " " << i2 << "/" << i2 << " " << i3 << "/" << i3 << std::endl;
		}

		//Write UVs
	}
	else
	{
		std::cout << "File cannot be found, opened etc. \n";
	}

	f.close();
	out.close();
}

typedef std::function<void(int, char* [])> procFunc;

std::string programName;
std::map<std::string, procFunc> processes;

void help()
{

}

void helpFunc(const std::string& func)
{

}

void populateProcs()
{
	processes.insert(std::make_pair("BM", [=](int argc, char* argv[]) {BMfunc(argc, argv, false); }));
	processes.insert(std::make_pair("BSM", [=](int argc, char* argv[]) {BMfunc(argc, argv, true); }));
}

int main(int argc, char* argv[])
{
	if (argc <= 0) { std::cout << "Uh oh"; return -1; }
    programName = argv[0];
	if (argc == 1)
	{
		std::cout << "No arguments provided! \n";
		help();
	}
	else
	{
		populateProcs();

		auto it = processes.find(argv[1]);
		if (it == processes.end()) { std::cout << "Cannot find func! \n"; helpFunc(argv[1]); }
		else
		{
			it->second(argc - 2, argv + 2);
		}
	}

	system("PAUSE");
	return NULL;
}