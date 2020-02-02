
#include "stdafx.h"
#include "GenotypeScanner.h"
#include "GenotypeParser.h"

using namespace evc;
using namespace evc::genotype_parser;

namespace evc
{
	// lookup table of reserved words
	struct sReservedWord {
		const char *str;
		Tokentype tok;
	};
	sReservedWord reservedWords[] =
	{
		{"#if", IF },
		{"#elif", IF },
		{"#else", ELSE },
		{"#array", ARRAY },
		{"#list", LIST },
		{"#tree", TREE},
		{"children", CHILDREN},
		{"preview", PREVIEW},
		{"stringview", STRINGVIEW},
		{"#", SHARP_BRACKET}, // 가장 낮은 우선순위로 검색해야한다.
	};
	const int g_rsvSize = sizeof(reservedWords) / sizeof(sReservedWord);
	Tokentype reservedLookup(const string &s);
}

Tokentype evc::reservedLookup( const string &s )
{
	int i=0;
	for (i=0; i < g_rsvSize; ++i)
	{
		if( !_stricmp(s.c_str(), reservedWords[ i].str) )
			return reservedWords[ i].tok;
	}
	return ID;
}


cScanner::cScanner() 
{
}

cScanner::~cScanner()
{
	Clear();
}


bool cScanner::LoadFile( const string &fileName
	, const bool bTrace //=false
)
{
	OFSTRUCT of;

	HFILE hFile = OpenFile(fileName.c_str(), &of, OF_READ);
	if (hFile != EOF)
	{
		const int fileSize = GetFileSize((HANDLE)hFile, NULL);
		if (fileSize <= 0)
			return false;

		DWORD readSize = 0;
		BYTE *mem = new BYTE[ fileSize+1];
		ReadFile((HANDLE)hFile, mem, fileSize, &readSize, NULL);
		CloseHandle((HANDLE)hFile);
		mem[ fileSize] = NULL;
		m_source = (char*)mem;
		delete[] mem;

		m_memSize = m_source.length();
		m_isTrace = bTrace;
		Initialize();
		return true;
	}

	dbg::Logc(1, "[%s] 파일이 없습니다.\n", fileName.c_str() );
	return false;
}



// load from memory
bool cScanner::LoadPackageFile(const BYTE *pFileMem, const int nFileSize )
{
	m_source.clear();
	m_source = (char*)pFileMem;
	m_memSize = m_source.length();
	Initialize();
	return true;
}


// is end?
bool cScanner::IsEnd()
{
	if (m_source.empty())
		return true;
	if (m_currentMemPoint >= m_memSize)
		return true;
	return false;
}


Tokentype cScanner::GetToken()
{
	if (m_source.empty())
	{
		return ENDFILE;
	}

	if (m_tokQ.size() == 0)
	{
		for( int i=0; i < MAX_QSIZE; ++i )
		{
			sTokDat d;
			string buf;
			d.tok = _GetToken( buf );
			d.str = buf;
			m_tokQ.push_back( d );
		}
	}
	else
	{
		sTokDat d;
		string buf;
		d.tok = _GetToken( buf );
		d.str = buf;
		m_tokQ.push_back( d );
		m_tokQ.pop_front();
	}

	return m_tokQ[ 0].tok;
}


Tokentype cScanner::GetTokenQ(const int nIdx )
{
	if (m_source.empty())
		return ENDFILE;
	return m_tokQ[ nIdx].tok;
}


const string& cScanner::GetTokenStringQ(const int nIdx )
{
	static string emptyString;
	if (m_source.empty())
		return emptyString;
	return m_tokQ[ nIdx].str;
}


char cScanner::GetNextChar()
{
	if (m_linePos >= m_memSize)
		return EOF;

	const short c = m_source[ m_linePos];
	++m_linePos;
	if (c == '\r')
		++m_lineNo;
	return (char)c;
}


// return scan string, same as fgets
bool cScanner::GetString(OUT char *receiveBuffer, int maxBufferLength)
{
	if (m_currentMemPoint >= m_memSize)
		return false;

	ZeroMemory(receiveBuffer, maxBufferLength);
	int i=0;
	for (i=0; (i < maxBufferLength) && (m_currentMemPoint < m_memSize); ++i)
	{
		const int memPoint = m_currentMemPoint;

		if ( '\r' == m_source[ memPoint] )
		{
			m_currentMemPoint += 2;
			receiveBuffer[ i++] = '\n';
			break;
		}
		else
		{
			receiveBuffer[ i] = m_source[ memPoint];
			++m_currentMemPoint;
		}
	}

	return true;
}


void cScanner::UngetNextChar()
{
	--m_linePos;
	if (m_linePos < 0)
		m_linePos = 0;
}


Tokentype cScanner::_GetToken( string &token )
{
	bool bFloat = false;
	Tokentype currentToken;
	StateType state = START;

	int nTok = 0;
	while (DONE != state)
	{
		char c = GetNextChar();
		bool save = true;
		switch( state )
		{
		case START:
			if ((c >= 0) && (c < 255) && isdigit(c))
				state = INNUM;
			else if ( ((c >=0) && (c < 255) && isalpha(c)) || '@' == c || '_' == c || '#' == c || '$' == c)
				state = INID;
			else
			{
				switch (c)
				{
				case '=': state = INEQ; save = false; break;
				case '!': state = INNEQ; save = false; break;
				case  '|': state = INOR; save = false; break;
				case '&': state = INAND; save = false; break;
				case '"': state = INSTR; save = false; break;
				case '/': state = INDIV; save = false; break;
				case '$': state = INCOMMENT; save = false; break;
				case '<': state = INLTEQ; save = false; break;
				case '>': state = INRTEQ; save = false; break;
				case '+': state = INPLUS; save = true; break;
				case '-': state = INMINUS; save = true; break;

				case ' ': // got through
				case '\t':
				case '\n':
				case '\r':
					save = false;
					break;

				default:
					{
						state = DONE;
						switch( c )
						{
						case EOF:
							save = false;
							currentToken = ENDFILE;
							break;
						case '*': currentToken = TIMES; break;
						//case '+': currentToken = PLUS; break;
						//case '-': currentToken = MINUS; break;
						case '%': currentToken = REMAINDER; break;
						//case '<': currentToken = RT; break;
						//case '>': currentToken = LT; break;
						case '(': currentToken = LPAREN; break;
						case ')': currentToken = RPAREN; break;
						case '{': currentToken = LBRACE; break;
						case '}': currentToken = RBRACE; break;
						case '[': currentToken = LBRACKET; break;
						case ']': currentToken = RBRACKET; break;
						case ',': currentToken = COMMA; break;
						//case ':':	currentToken = COLON; break;
						case ';': currentToken = SEMICOLON; break;
						default: currentToken = _ERROR; break;
						}
					}
					break;
				}
			}
			break;

		case INCOMMENT:
			save = false;
			if ('\n' == c) 
				state = START;
			break;

		case INMULTI_COMMENT:
			if('*' == c)
			{
				state = OUTMULTI_COMMENT;
			}
			break;

		case OUTMULTI_COMMENT:
			if ('/' == c)
			{
				state = START;
			}
			else
			{
				state = INMULTI_COMMENT;
			}
			break;

		case INNUM:
			if ((c >= 0) && (c <= 255) && !isdigit(c) && ('.' != c) && ('-' != c) && ('+' != c))
			{
				UngetNextChar();
				save = false;
				state = DONE;
				currentToken = NUM;
			}
			break;
		case INID:
			if ((c >= 0) && (c <= 255) && !isalpha(c) && !isdigit(c) && 
				 ('_' != c) && ('.' != c) && ('@' != c) && ('-' != c) && (':' != c) && ('/'!=c) )
			{
				UngetNextChar();
				save = false;
				state = DONE;
				currentToken = ID;
			}
			break;
		case INDIV:
			if ('/' == c)
			{
				state = INCOMMENT;
			}
			else if ('*' == c)
			{
				state = INMULTI_COMMENT;
			}
			else
			{
				UngetNextChar();
				currentToken = DIV;
				state = DONE;
			}
			save = false;
			break;

		case INEQ:
			if ('=' == c)
			{
				currentToken = EQ;
			}
			else
			{
				UngetNextChar();
				currentToken = ASSIGN;
			}
			save = false;
			state = DONE;
			break;
		case INNEQ:
			if ('=' == c)
			{
				currentToken = NEQ;
			}
			else
			{
				currentToken = NEG;
				UngetNextChar();
			}
			save = false;
			state = DONE;
			break;
		case INOR:
			if ('|' == c)
			{
				currentToken = OR;
			}
			else
			{
				UngetNextChar();
				currentToken = LOGIC_OR;
			}
			save = false;
			state = DONE;
			break;
		case INAND:
			if ('&' == c)
			{
				currentToken = AND;
			}
			else
			{
				currentToken = REF;
				UngetNextChar();
			}
			save = false;
			state = DONE;
			break;
		case INSTR:
			if ('"' == c)
			{
				save = false;
				state = DONE;
				currentToken = STRING;
			}
			break;

		case INARROW:
			if ('>' == c)
			{
				currentToken = ARROW;
			}
			else if ('-' == c)
			{
				currentToken = DEC;
			}
			else
			{
				UngetNextChar();
				currentToken = MINUS;
			}
			save = false;
			state = DONE;
			break;

		case INPLUS:
			if ('+' == c)
			{
				currentToken = INC;
				save = false;
				state = DONE;
			}
			else if ((c >= 0) && (c <= 255) && (isdigit(c) || ('.' == c)))
			{
				state = INNUM;
			}
			else
			{
				UngetNextChar();
				currentToken = PLUS;
				save = false;
				state = DONE;
			}
			break;

		case INMINUS:
			if ('-' == c)
			{
				currentToken = DEC;
				save = false;
				state = DONE;
			}
			else if ((c >= 0) && (c <= 255) && (isdigit(c) || ('.' == c)))
			{
				state = INNUM;
			}
			else
			{
				UngetNextChar();
				currentToken = MINUS;
				save = false;
				state = DONE;
			}
			break;

		case INLTEQ:
			if ('=' == c)
			{
				currentToken = LTEQ;
			}
			else
			{
				UngetNextChar();
				currentToken = LT;
			}
			save = false;
			state = DONE;
			break;

		case INRTEQ:
			if ('=' == c)
			{
				currentToken = RTEQ;
			}
			else
			{
				UngetNextChar();
				currentToken = RT;
			}
			save = false;
			state = DONE;
			break;

		case INSCOPE:
			{
				if (':' == c)
				{
					currentToken = SCOPE;
				}
				else
				{
					UngetNextChar();
					currentToken = COLON;
				}
				save = false;
				state = DONE;
			}
			break;

		case DONE:
		default:
			dbg::Logc(1, "scanner bug: state = %d", state );
			state = DONE;
			currentToken = _ERROR;
			break;
		}

		if (save)
		{
			token += (char)c;
		}
		if (DONE == state)
		{
			if (ID == currentToken)
				currentToken = reservedLookup(token);
		}
		if (EOF == c)
		{
			SetEndOfFile();
		}
	}

	if (m_isTrace)
	{
		dbg::Logc(1, "    %d: ", m_lineNo );
	}

	return currentToken;
}


void cScanner::SetEndOfFile()
{
	m_currentMemPoint = m_memSize;
	m_linePos = m_memSize;
}


void cScanner::Initialize()
{
	m_lineNo = 0;
	m_linePos = 0;
	m_currentMemPoint = 0;
}


void cScanner::Clear()
{
}


//-----------------------------------------------------------------------------
// genotype_parser
//-----------------------------------------------------------------------------
void genotype_parser::RemoveExpressoin_OnlyExpr(sExpr *expr)
{
	if (!expr)
		return;
	sConnectionList *pnode = expr->connection;
	while (pnode)
	{
		sConnectionList *rmNode = pnode;
		pnode = pnode->next;
		SAFE_DELETE(rmNode->connect);
		SAFE_DELETE(rmNode);
	}
	SAFE_DELETE(expr);
}


void genotype_parser::RemoveExpression(sExpr *expr)
{
	if (!expr)
		return;

	map<string,sExpr*> rm;
	std::queue<sExpr*> q;	
	q.push(expr);

	while (!q.empty())
	{
		sExpr *node = q.front(); q.pop();
		if (!node) 
			continue;
		if (rm.find(node->id) != rm.end())
			continue;

		rm[ node->id] = node;
		sConnectionList *con = node->connection;
		while (con)
		{
			q.push(con->connect->expr);
			con = con->next;
		}
	}

	for (auto &kv : rm)
	{
		RemoveExpressoin_OnlyExpr(kv.second);
	}
}


// return Copy Genotype
// 2014-02-27
genotype_parser::sExpr* CopyGenotypeRec(const sExpr *expr, map<string,sExpr*> &symbols)
{
	RETV(!expr, NULL);

	auto it = symbols.find(expr->id);
	if (symbols.end() != it)
	{ // already exist
		return it->second;
	}

	sExpr *newExpr = new sExpr;
	*newExpr = *expr;
	symbols[ newExpr->id] = newExpr;

	sConnectionList *srcConnection = expr->connection;
	sConnectionList *currentDestConnection = NULL;
	while (srcConnection)
	{
		if (!currentDestConnection)
		{
			currentDestConnection = new sConnectionList;
			newExpr->connection = currentDestConnection;
		}
		else
		{
			sConnectionList *newCopy = new sConnectionList;
			currentDestConnection->next = newCopy;
			currentDestConnection = newCopy;
		}

		currentDestConnection->connect = new sConnection;
		*currentDestConnection->connect = *srcConnection->connect;
		currentDestConnection->connect->expr = CopyGenotypeRec(srcConnection->connect->expr, symbols);

		srcConnection = srcConnection->next;
	}

	return newExpr;
}


// return Copy Genotype
// 2014-02-27
sExpr* genotype_parser::CopyGenotype(const sExpr *expr)
{
	map<string,sExpr*> symbols;
	return CopyGenotypeRec(expr, symbols);
}


// 2014-03-05
void AssignGenotypeRec(sExpr *dest, const sExpr *src, set<string> &symbols)
{
	RET(!dest || !src);

	auto it = symbols.find(dest->id);
	if (symbols.end() != it)
	{ // already exist
		return;
	}

	*dest = *src;
	symbols.insert(dest->id);

	sConnectionList *srcConList = src->connection;
	sConnectionList *destConList = dest->connection;
	while (srcConList && destConList)
	{
		*destConList->connect = *srcConList->connect;
		AssignGenotypeRec(destConList->connect->expr, srcConList->connect->expr, symbols);

		srcConList = srcConList->next;
		destConList = destConList->next;
	}
}


// assign deep genotype
// 2014-03-05
void genotype_parser::AssignGenotype(sExpr *dest, const sExpr *src)
{
	set<string> symbols;
	AssignGenotypeRec(dest, src, symbols);
}


// FindGenotype
// 2014-03-05
sExpr* FindGenotypeRec(sExpr *expr, const string &id, set<string> &symbols)
{
	RETV(!expr, NULL);

	auto it = symbols.find(expr->id);
	if (symbols.end() != it)
	{ // already exist
		return NULL;
	}

	if (boost::iequals(expr->id, id))
		return expr;

	symbols.insert(expr->id);

	sConnectionList *conList = expr->connection;
	while (conList)
	{
		if (sExpr *ret = FindGenotypeRec(conList->connect->expr, id, symbols))
			return ret;
		conList = conList->next;
	}
	return NULL;
}


// 2014-03-05
sExpr* genotype_parser::FindGenotype(sExpr *expr, const string &id)
{
	set<string> symbols;
	return FindGenotypeRec(expr, id, symbols);
}
