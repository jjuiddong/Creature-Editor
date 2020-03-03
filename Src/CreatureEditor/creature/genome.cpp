
#include "stdafx.h"
#include "genome.h"

using namespace evc;


cGenome::cGenome()
{
}

cGenome::~cGenome()
{
	Clear();
}


// read genome file
bool cGenome::Read(const StrPath &fileName)
{
	using namespace std;

	Clear();

	ifstream ifs(fileName.c_str(), std::ios::binary);
	if (!ifs.is_open())
		return false;

	char fmt[4] = { '\0', '\0', '\0', '\0' };
	ifs.read(fmt, 3);
	if (strcmp(fmt,"GEN"))
		return false; // fail file format

	ifs.read(m_name.m_str, m_name.SIZE);
	int gsize = 0;
	ifs.read((char*)&gsize, sizeof(gsize));

	m_genomes.resize(gsize);

	for (int i = 0; i < gsize; ++i)
	{
		int size = 0;
		ifs.read((char*)&size, sizeof(size));
		m_genomes[i].chromo.resize(size);
		ifs.read((char*)&m_genomes[i].layerCnt, sizeof(m_genomes[i].layerCnt));
		ifs.read((char*)&m_genomes[i].inputCnt, sizeof(m_genomes[i].inputCnt));
		ifs.read((char*)&m_genomes[i].outputCnt, sizeof(m_genomes[i].outputCnt));
		ifs.read((char*)&m_genomes[i].chromo[0], sizeof(double) * size);
	}

	return true;
}


// write genome data
bool cGenome::Write(const StrPath &fileName)
{
	using namespace std;

	ofstream ofs(fileName.c_str(), std::ios::binary);
	if (!ofs.is_open())
		return false;

	ofs.write("GEN", 3); // file format
	ofs.write(m_name.m_str, m_name.SIZE);
	const int gsize = (int)m_genomes.size();
	ofs.write((char*)&gsize, sizeof(gsize));

	for (auto &genome : m_genomes)
	{
		const int size = (int)genome.chromo.size();
		ofs.write((char*)&size, sizeof(size));
		ofs.write((char*)&genome.layerCnt, sizeof(genome.layerCnt));
		ofs.write((char*)&genome.inputCnt, sizeof(genome.inputCnt));
		ofs.write((char*)&genome.outputCnt, sizeof(genome.outputCnt));
		if (!genome.chromo.empty())
			ofs.write((char*)&genome.chromo[0], sizeof(double) * size);
	}

	return true;
}


void cGenome::Clear()
{
	m_genomes.clear();
}
