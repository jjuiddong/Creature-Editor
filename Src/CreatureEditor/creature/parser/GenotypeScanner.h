//
// 2013-12-05, jjuiddong
//	- Genotype Script Scanner
//
// 2020-02-02
//	- refactoring
//
#pragma once


namespace evc { namespace genotype_parser {

	enum Tokentype
	{
		_ERROR, NONE, ENDFILE, ID, NUM, FNUM, STRING, ASSIGN, LPAREN, RPAREN, 
		LBRACE/*{*/, RBRACE/*}*/, 
		LBRACKET/* [ */, RBRACKET/* ] */, 
		DOT, COMMA, COLON, SEMICOLON,
		PLUS, MINUS, TIMES, DIV, REMAINDER, REF, ARROW, INC, DEC,
		LT/* < */, RT/* > */, LTEQ/* <= */, RTEQ/* >= */, NEQ/* != */, EQ/* == */, LOGIC_OR/* | */, OR/* || */, AND/* && */, NEG/* ! */, SCOPE/*::*/,

		// special
		IF, ELIF, ELSE, ARRAY, LIST,  TREE, CHILDREN, PREVIEW, STRINGVIEW,
		SHARP_BRACKET, 
	};


	struct sExpr;
	struct sConnection
	{
		string conType; // joint, sensor
		string type; // joint type
		Vector3 jointAxis;
		Vector3 jointPos;
		Vector3 pivot0;
		Vector3 pivot1;
		Transform conTfm0; // connection rigidbody transform
		Transform conTfm1; // connection rigidbody transform
		Quaternion rotate;
		Vector2 drive; // velocity, period
		Vector4 limit; // cone limit (yAngle, zAngle)
						// angular limit (lower, upper)
						// linear limit (lower, upper, stiffness, damping)
						// distance limit (min, max)
		Vector3 twistLimit;
		Vector3 swingLimit;
		bool terminalOnly;
		string exprName;
		sExpr *expr;

		sConnection() {
			conType = "joint";
			type = "fixed";
			jointAxis = Vector3(0,1,0);
			terminalOnly = false;
			expr = NULL;
		}

		const sConnection& operator=(const sConnection &rhs) {
			if (this != &rhs)
			{
				conType = rhs.conType;
				type = rhs.type;
				jointAxis = rhs.jointAxis;
				rotate = rhs.rotate;
				limit = rhs.limit;
				terminalOnly = rhs.terminalOnly;
			}
			return *this;
		}
	};

	struct sConnectionList
	{
		sConnection *connect;
		sConnectionList *next;
		sConnectionList() : connect(NULL), next(NULL) {}
	};

	struct sExprList;
	struct sExpr
	{
		string id;
		string shape;
		Vector3 dimension;
		Vector3 material;
		float density;
		sConnectionList *connection;
		int refCount;
		bool isSensor; // internal used

		sExpr() {
			density = 1.f;
			material = Vector3(0,0.75f,0);
			connection = NULL;
			isSensor = false;
		}

		const sExpr& operator=(const sExpr &rhs) {
			if (this != &rhs)
			{
				id = rhs.id;
				shape = rhs.shape;
				dimension = rhs.dimension;
				material = rhs.material;
				density = rhs.density;
				isSensor = rhs.isSensor;
			}
			return *this;
		}
	};

	struct sExprList
	{
		sExpr *expr;
		sExprList *next;
	};


	class cScanner
	{
	public:
		cScanner();
		virtual ~cScanner();

		bool LoadFile( const string &fileName, const bool bTrace=false );
		bool LoadPackageFile(const BYTE *pFileMem, const int nFileSize );
		Tokentype GetToken();
		Tokentype GetTokenQ(const int nIdx );
		const string& GetTokenStringQ(const int nIdx );
		int GetLineNo() { return m_lineNo; }
		bool IsEnd();
		void Clear();


	protected:
		void Initialize();
		char GetNextChar();
		void UngetNextChar();
		Tokentype _GetToken( string &pToken );
		bool GetString(OUT char *receiveBuffer, int maxBufferLength);
		void SetEndOfFile();


	private:
		enum { MAX_QSIZE=8 };
		enum StateType { START, INASSIGN, INCOMMENT, INMULTI_COMMENT, OUTMULTI_COMMENT, 
			INNUM, INID, INSTR, INDIV, INEQ, INNEQ, INOR, INAND, INLTEQ, INRTEQ, INARROW, INSCOPE, INSHARP, 
			INPLUS, INMINUS,
			DONE };

		struct sTokDat
		{
			string str;
			Tokentype tok;
		};

		std::string m_source;
		int m_currentMemPoint;
		int m_memSize;
		int m_lineNo;
		int m_linePos;
		std::deque<sTokDat> m_tokQ;
		bool m_isTrace;
	};


	void RemoveExpression(sExpr *expr);
	void RemoveExpressoin_OnlyExpr(sExpr *expr);
	sExpr* CopyGenotype(const sExpr *expr);
	void AssignGenotype(sExpr *dest, const sExpr *src);
	sExpr* FindGenotype(sExpr *expr, const string &id);

}}

