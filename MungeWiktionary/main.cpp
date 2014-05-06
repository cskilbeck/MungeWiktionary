// wove
// 
// super/subscript
// maths
// slang
// duplicate definitions
// order definitions by whether they have certain qualifiers (obsolete to the back, for example)
//////////////////////////////////////////////////////////////////////

#include "pch.h"

using std::regex;
using std::regex_replace;
using std::sregex_iterator;
using std::smatch;
using std::map;
using std::unordered_map;
using std::pair;

#define STRLEN(x) x, sizeof(x) - 1

//////////////////////////////////////////////////////////////////////

std::set<string> gEnable;

//////////////////////////////////////////////////////////////////////

static inline std::string &rtrim(std::string &s)
{
	for(auto i = s.begin(); i != s.end(); ++i)
	{
		if(*i == '\n')
		{
			s.erase(i, s.end());
			return s;
		}
	}
	return s;
}

//////////////////////////////////////////////////////////////////////

void LoadEnable()
{
	FILE *f = fopen("enable.txt", "r");
	if(f != null)
	{
		string s;
		char buffer[128];
		while(!feof(f))
		{
			if(fgets(buffer, sizeof(buffer) - 1, f) != null)
			{
				s = buffer;
				s = rtrim(s);
				gEnable.insert(s);
			}
		}
		fclose(f);
	}
}

//////////////////////////////////////////////////////////////////////

struct Reference
{
	enum eType
	{
		kNone,
		kPastTense,
		kPlural,
		kComparative,
		kSuperlative,
		kObsoleteForm,
		kObsoleteSpelling,
		kAltCapitalization,
		kAltSpelling,
		kInformalSpelling,
		kArchaicSpelling,
		kArchaicForm,
		kArchaicThirdPersonSingular,
		kPresentTense,
		kThirdPersonSingular,
		kSecondPersonSingular,
		kMisspelling,
		kAltForm,
		kDatedForm,
		kObsoleteTypography,
		kEyeDialect,
		kAbbreviation,
		kNumReferenceTypes
	};

	static bool	sKillTable[kNumReferenceTypes];

	eType		mType;
	string		mReferenceWord;

	static Reference sReference[];

	Reference(eType type = kNone) : mType(type)
	{
	}
};

bool Reference::sKillTable[Reference::kNumReferenceTypes] =
{
			//		Keep					Kill
			//========================================================
	false,	//		kNone,
	false,	//		kPastTense,
	false,	//		kPlural,
	false,	//		kComparative,
	false,	//		kSuperlative,
	true,	//								kObsoleteForm,
	true,	//								kObsoleteSpelling,
	false,	//		kAltCapitalization,
	false,	//		kAltSpelling,
	false,	//		kInformalSpelling,
	true,	//								kArchaicSpelling,
	true,	//								kArchaicForm,
	true,	//								kArchaicThirdPersonSingular
	false,	//		kPresentTense,
	false,	//		kThirdPersonSingular,
	false,	//		kSecondPersonSingular,
	true,	//								kMisspelling,
	false,	//		kAltForm,
	false,	//		kDatedForm,
	true,	//								kObsoleteTypography,
	true,	//								kEyeDialect,
	true	//								kAbbreviation

			//		kNumReferenceTypes
};

//////////////////////////////////////////////////////////////////////

struct Definition
{
	enum eType
	{
		kInvalid			= -1,
		kNoun				=  0,
		kVerb				=  1,
		kAdjective			=  2,
		kAdverb				=  3,
		kConjunction		=  4,
		kPronoun			=  5,
		kPreposition		=  6,
		kNumeral			=  7,
		kNumDefinitionTypes	=  8
	};

	Reference			mReference;
	string				mText;

	Definition() : mReference()
	{
	}

	static char const *sDefinitionNames[kNumDefinitionTypes];

	//////////////////////////////////////////////////////////////////////

	static inline Definition::eType GetDefinitionType(char const *desc)
	{
		if(strnicmp(desc, STRLEN("noun")) == 0)			return kNoun;
		if(strnicmp(desc, STRLEN("verb")) == 0)			return kVerb;
		if(strnicmp(desc, STRLEN("adjective")) == 0)	return kAdjective;
		if(strnicmp(desc, STRLEN("adverb")) == 0)		return kAdverb;
		if(strnicmp(desc, STRLEN("conjunction")) == 0)	return kConjunction;
		if(strnicmp(desc, STRLEN("pronoun")) == 0)		return kPronoun;
		if(strnicmp(desc, STRLEN("preposition")) == 0)	return kPreposition;
		if(strnicmp(desc, STRLEN("numeral")) == 0)		return kNumeral;
		return kInvalid;
	}

};

//////////////////////////////////////////////////////////////////////

struct Word
{
	string				mWord;
	bool				mActive;
	uint32				mIndex;
	list<Definition *>	mDefinition[Definition::kNumDefinitionTypes];

	//////////////////////////////////////////////////////////////////////

	bool GotDefinition(int t)
	{
		return !mDefinition[t].empty();
	}

	//////////////////////////////////////////////////////////////////////

	bool GotAnyDefinitions()
	{
		for(int t=0; t<Definition::kNumDefinitionTypes; ++t)
		{
			if(GotDefinition(t))
			{
				return true;
			}
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////

	bool Got3Definitions()
	{
		int count = 0;
		for(int t=0; t<Definition::kNumDefinitionTypes; ++t)
		{
			if(GotDefinition(t))
			{
				++count;
			}
		}
		return count > 3;
	}

	//////////////////////////////////////////////////////////////////////

	Word(char const *s) : mWord(s)
	{
	}

	~Word()
	{
		for(int t = 0; t < Definition::kNumDefinitionTypes; ++t)
		{
			while(!mDefinition[t].empty())
			{
				Definition *p = mDefinition[t].front();
				delete p;
				mDefinition[t].pop_front();
			}
		}
	}
};

//////////////////////////////////////////////////////////////////////

char const *Definition::sDefinitionNames[Definition::kNumDefinitionTypes] =
{
	"n",
	"v",
	"adj",
	"adv",
	"conj",
	"pron",
	"prep"
};

//////////////////////////////////////////////////////////////////////

map<string, Word *>	gWords;

//////////////////////////////////////////////////////////////////////

struct Replacer
{
	regex				mRegex;
	string				mReplacerString;
	Reference::eType	mReferenceType;

	Replacer(char const *rgx, char const *replacer, Reference::eType refType = Reference::eType::kNone) : mRegex(rgx), mReplacerString(replacer), mReferenceType(refType)
	{
	}

	string SimpleReplace(string const &str)
	{
		return regex_replace(str, mRegex, mReplacerString);
	}

	// inout str - current definition
	// out refWord - word being referenced (if it's a reference)
	// out refType - what type of reference it is (might be kNone, in which case refWord will be unmodified)

	bool DoReplacement (string &str, string &refWord, Reference::eType &refType)
	{
		if(regex_search(str, mRegex))
		{
			refType = mReferenceType;
			if(mReferenceType != Reference::eType::kNone)
			{
				smatch m;
				regex_match(str, m, mRegex);
				refWord = m[1].str();
			}
			str = regex_replace(str, mRegex, mReplacerString);
			return true;
		}
		return false;
	}
};

//////////////////////////////////////////////////////////////////////

struct TemplateCall
{
	typedef pair<string, string> tNameValue;

	vector<tNameValue>	mParameters;	// name,value

	static string empty;

	void Reset()
	{
		mParameters.clear();
	}

	void AddParameter(string const &name, string const &value)
	{
		mParameters.push_back(tNameValue(name, value));
	}
	
	string const &Parameter(int n) const
	{
		return (n < mParameters.size()) ? mParameters[n].second : empty;
	}

	string const &Name() const
	{
		return Parameter(0);
	}

	string ToString()
	{
		string result("(");
		string separator;
		for(auto i = mParameters.begin(); i != mParameters.end(); ++i)
		{
			result += separator;
			if(!i->first.empty())
			{
				result += i->first;
				result += "=";
			}
			result += i->second;
			separator = ",";
		}
		result += ")";
		return result;
	}
};

string TemplateCall::empty;

//////////////////////////////////////////////////////////////////////

struct TemplateReplacer
{
	char const *		mTemplateName;
	char const *		mReplacementString;
	Reference::eType	mReferenceType;
};

//////////////////////////////////////////////////////////////////////

TemplateReplacer gTemplateReplacements[] =
{
	"chiefly",							"(Chiefly %1)",								Reference::eType::kNone,
	"defdate",							"",											Reference::eType::kNone,
	"Latn-def",							"The name of the Latin script letter '%3'",	Reference::eType::kNone,
	"taxlink",							"%1",										Reference::eType::kNone,
	"term"	,							"%1",										Reference::eType::kNone,
	"non-gloss definition",				"%1",										Reference::eType::kNone,
	"unsupported",						"%1",										Reference::eType::kNone,
	"l",								"%2",										Reference::eType::kNone,
	"rfd-sense",						"",											Reference::eType::kNone,
	"n-g",								"%1",										Reference::eType::kNone,
	"rfd-redundant",					"",											Reference::eType::kNone,
	"w",								"",											Reference::eType::kNone,
	"form of",							"%1 form of @%2@",							Reference::eType::kNone,

	"alternative form of",				"Alt. form of @%1@",						Reference::eType::kAltForm,
	"past of",							"Past tense of @%1@",						Reference::eType::kPastTense,
	"simple past of",					"Past tense of @%1@",						Reference::eType::kPastTense,
	"past participle of",				"Past tense of @%1@",						Reference::eType::kPastTense,
	"plural of",						"Plural of @%1@",							Reference::eType::kPlural,
	"irregular plural of",				"Plural of @%1@",							Reference::eType::kPlural,
	"comparative of",					"Comparative of @%1@",						Reference::eType::kComparative,
	"superlative of",					"Superlative of @%1@",						Reference::eType::kSuperlative,
	"obsolete form of",					"Obsolete form of @%1@",					Reference::eType::kObsoleteForm,
	"obsolete spelling of",				"Obsolete spelling of @%1@",				Reference::eType::kObsoleteSpelling,
	"alternative capitalization of",	"Alt. capitalization of @%1@",				Reference::eType::kAltCapitalization,
	"alternative spelling of",			"Alt. spelling of @%1@",					Reference::eType::kAltSpelling,
	"informal spelling of",				"Informal spelling of @%1@",				Reference::eType::kInformalSpelling,
	"alternate spelling of",			"Alt. spelling of @%1@",					Reference::eType::kAltSpelling,
	"archaic third-person singular of",	"Archaic 3rd person singular of @%1@",		Reference::eType::kArchaicThirdPersonSingular,
	"second-person singular of",		"2nd person singular of",					Reference::eType::kSecondPersonSingular,
	"archaic spelling of",				"Archaic spelling of @%1@",					Reference::eType::kArchaicSpelling,
	"archaic form of",					"Archaic form of @%1@",						Reference::eType::kArchaicForm,
	"present participle of",			"Present tense of @%1@",					Reference::eType::kPresentTense,
	"third-person singular of",			"3rd person singular of @%1@",				Reference::eType::kThirdPersonSingular,
	"alternative plural of",			"Plural of @%1@",							Reference::eType::kPlural,
	"irregular plural of",				"Plural of @%1@",							Reference::eType::kPlural,
	"misspelling of",					"Misspelling of @%1@",						Reference::eType::kMisspelling,
	"alternate form of",				"Alt. form of @%1@",						Reference::eType::kAltForm,
	"dated form of",					"Dated form of @%1@",						Reference::eType::kDatedForm,
	"eye dialect of",					"Eye dialect of @%1@",						Reference::eType::kEyeDialect,
	"obsolete typography of",			"Obsolete typography of @1@",				Reference::eType::kObsoleteTypography,
	"plural of",						"Plural of @%1@",							Reference::eType::kPlural,
	"abbreviation of",					"Abbreviation of @%1@",						Reference::eType::kAbbreviation
};

//////////////////////////////////////////////////////////////////////

bool GetTemplateReplacement(TemplateCall &templateCall, string &result, string &refWord, Reference::eType &referenceType)
{
	for(int i=0; i<ARRAYSIZE(gTemplateReplacements); ++i)
	{
		TemplateReplacer &t = gTemplateReplacements[i];
		char const *p = templateCall.Name().c_str();

		if(strnicmp(p, "en-", 3) == 0)
		{
			p += 3;
		}

		if(stricmp(gTemplateReplacements[i].mTemplateName, p) == 0)
		{
			// replace $1 with 1st parameter value, $2 with 2nd and so on
			result.clear();

			referenceType = t.mReferenceType;

			if(t.mReplacementString[0] != '\0')
			{
				char const *p = t.mReplacementString;
				char c;
				while((c = *p++) != 0)
				{
					if(c == '%')
					{
						int param = *p++ - '1' + 1;
						result += templateCall.Parameter(param);
					}
					else
					{
						result += c;
					}
				}
				refWord = templateCall.Parameter(1);
			}
			return true;
		}
	}

	static string comma(",");
	static string or(" or ");
	static string and(" and ");
	static string space(" ");
	static string ofa(" of a ");

	string *separator = &comma;

	// deal with it as context
	string const &name = templateCall.Name();
	int startpos = -1;
	if(name == "context")
	{
		startpos = 1;
	}
	else if(name == "archaic" || name == "obsolete")
	{
		startpos = 0;
		separator = &space;
	}
	else
	{
		startpos = 0;
	}

	if(startpos >= 0)
	{
		// handle context
		result = "(";
		referenceType = Reference::eType::kNone;

		bool empty = true;

		for(int i = startpos; i<templateCall.mParameters.size(); ++i)
		{
			string name = templateCall.mParameters[i].first;
			string value = templateCall.mParameters[i].second;

			// remove all named parameters
			if(!name.empty())
			{
				continue;
			}

			if(value == "_")
			{
				separator = &space;
			}
			else if(value == "or")
			{
				separator = &or;
			}
			else if(value == "and")
			{
				separator = &and;
			}
			else if(value == "of a")
			{
				separator = &ofa;
			}
			else
			{
				if(i != startpos)
				{
					result += *separator;
				}

				separator = &comma;
				result += value;
				empty = false;
			}
		}
		if(empty && startpos == 0)
		{
			result += name;
		}
		result += ")";
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////

void TrimString(string &str)
{
	while(true)
	{
		char c = str.back();
		if(c == ' ' || c == '\t')
		{
			str.pop_back();
		}
		else
		{
			break;
		}
	}

	int i = 0;
	while(true)
	{
		char c = str[i];
		if(c != ' ' && c != '\t')
		{
			break;
		}
		++i;
	}
	if(i != 0)
	{
		str = str.substr(i);
	}
}

//////////////////////////////////////////////////////////////////////

string HandleTemplates(string str, string &refWord, Reference::eType &refType)
{
	// find the templates with this RX
	static regex braceMatch("\\{\\{\\s*(.*?)\\s*\\}\\}");

	sregex_iterator it(str.begin(), str.end(), braceMatch);
	sregex_iterator end;

	string result;
	string suffix;

	refType = Reference::kNone;

	if(it == end)
	{
		return str;
	}

	// loop over all the template calls in the string
	for(; it != end; ++it)
	{
		TemplateCall t;

		result += it->prefix();

		string &str = (*it)[1].str();

		// find the template parts with this RX
		static regex barSplit("\\s*([^$|]+)\\s*[$|]?");

		sregex_iterator bit(str.begin(), str.end(), barSplit);
		sregex_iterator bend;

		bool begin = true;

		// loop over the template parts
		for(; bit != bend; ++bit)
		{
			string m = (*bit)[1].str();
			TrimString(m);

			// 1st template part is the template name
			if(begin)
			{
				t.AddParameter(string(), m);
				begin = false;
			}
			else
			{
				// subsequent pieces are parameters
				static regex equalsSplit("^([^=]*)(=?)(.*)");
				smatch equalsMatch;

				if(regex_match(m, equalsMatch, equalsSplit))
				{
					string name;
					string value;
					if(equalsMatch[2].str().empty())
					{
						// either an unnamed parameter
						value = equalsMatch[1].str();
					}
					else
					{
						// or a named one
						name = equalsMatch[1].str();
						value = equalsMatch[3].str();
					}

					if(name != "lang" && name != "nodot")
					{
						t.AddParameter(name, value);
					}
				}
			}
		}

		// we have a TemplateCall
		// Deal with it, and

		string rp;

		if(GetTemplateReplacement(t, rp, refWord, refType))
		{
			result += rp;
		}
		else
		{
			result += t.ToString();
		}
		suffix = it->suffix();
	}
	result += suffix;
	return result;
}

Replacer replacements[] =
{
	Replacer("<ref(.*?)/?(.*)((/>)|(</ref>))",						""),
	Replacer("<!--(.*?)-->",										""),
	Replacer("<!--(.*)",											""),	// buggy trailing chopped comments
	Replacer("__",													""),
	Replacer("'(.*?)'",												"$1"),
	Replacer("''(.*?)''",											"$1"),
	Replacer("'''(.*?)'''",											"$1"),
	Replacer("\\[\\[w:(.*?)\\|(.*?)\\]\\]",							"$1"),
	Replacer("\\[\\[.*?#(.*?)\\|(.*?)\\]\\]",						"$2"),
	Replacer("\\{\\{n\\-g\\|(.*?)\\}\\}",							"$1"),
	Replacer("\\{\\{rfc-sense\\}\\}",								""),
	Replacer("\\{\\{rfv-sense\\}\\}",								""),

	Replacer("''\\[\\[(.*?)\\|(.*?)\\]\\]''",						"$2"),
	Replacer("\\[\\[([^\\]]*?)\\|([^\\]]*?)\\]\\]",					"$1"),
	Replacer("\\[\\[([^ ]|[a-z]*?)\\]\\]",							"@$1@"),
	Replacer("\\[\\[(.*?)\\]\\]",									"$1"),
	Replacer("\\[(http.*?)\\], ",									""),
	Replacer("&nbsp;",												" "),
	Replacer("&lt;",												"<"),
	Replacer("&gt;",												">"),
	Replacer("&amp;",												"&"),
	Replacer("&cent;",												"¢"),
	Replacer("&pound;",												"£"),
	Replacer("&yen;",												"¥"),
	Replacer("&euro;",												"€"),
	Replacer("&copy;",												"©"),
	Replacer("&reg;",												"®"),
	Replacer("&trade;",												"™"),
	Replacer("&mdash;",												"-"),
	Replacer("&dagger;",											"->--"),

	Replacer("<math>(.*?)</math>",									"[some heavy maths]"),
	Replacer("<div.*?>(.*?)</div>",									"$1"),
	Replacer("\\\"",												"'"),
	Replacer("@@",													"@")
};

//////////////////////////////////////////////////////////////////////

Replacer formattingReplacements[] =
{
	Replacer("__",													""),
	Replacer("'(.*?)'",												"$1"),
	Replacer("''(.*?)''",											"$1"),
	Replacer("'''(.*?)'''",											"$1"),
	Replacer("\"(.*?)\"",											"'$1'"),
	Replacer("\\[\\[([^ ]|[a-z]*?)\\]\\]",							"$1")
};

string StripFormatting(string s)
{
	for(int i=0; i<ARRAYSIZE(formattingReplacements); ++i)
	{
		s = formattingReplacements[i].SimpleReplace(s);
	}
	return s;
}

//////////////////////////////////////////////////////////////////////

string ParseDefinition(Definition &def, string const &word, string const &body, bool &killIt)
{
	smatch mr;
	string str = body;

	if(strstr(str.c_str(), "{{rfdef") != null)
	{
		killIt = true;
		return string();
	}

	Reference::eType refType;
	string refWord;
	str = HandleTemplates(str, refWord, refType);

	if(refType != Reference::eType::kNone)
	{
		def.mReference.mReferenceWord = StripFormatting(refWord);
		def.mReference.mType = refType;
	}

	for(int i=0; i<ARRAYSIZE(replacements); ++i)
	{
		replacements[i].DoReplacement(str, refWord, refType);
	}

	def.mText = str;

	while(!str.empty())
	{
		char c = str.back();
		if(c == '.' || c == '\r' || c == '\n' || c == ' ')
		{
			str.pop_back();
		}
		else
		{
			break;
		}
	}

	int n = 0;
	for(int n=0; n<str.length(); ++n)
	{
		char c = str[n];
		if(c != ',' && c != ' ' && c != '.')
		{
			break;
		}
	}
	if(n > 0)
	{
		str = str.substr(n);
	}

	if(str.empty())
	{
		fprintf(stderr, "Couldn't parse:[%s]:@%s@\n\n", word.c_str(), body.c_str());
		return body;
	}
	return str;
}

//////////////////////////////////////////////////////////////////////

static regex lineRX("English\\t([^\\t]*)\\t([^\\t]*)\\t# ?(.*)");

static int lines = 0;

void ProcessLine(string const &s)
{
	if(++lines % 1000 == 0)
	{
		TRACE("%s\n", s.c_str());
	}
	smatch match;
	if(regex_match(s, match, lineRX))
	{
		string const &word = match[1].str();
		string const &wordType = match[2].str();
		string const &definition = match[3].str();

		Definition::eType definitionType = Definition::GetDefinitionType(wordType.c_str());

		if(definitionType != Definition::kInvalid)
		{
			size_t l = word.size();
			for(int i=0; i<l; ++i)
			{
				char c = word[i];
				if(c <'a' || c >'z')
				{
					return;
				}
			}

			Word *n = gWords[word];

			if(n == null)
			{
				n = new Word(word.c_str());
				gWords[word] = n;
			}

			bool killIt = false;
			bool isReference = false;
			
			Definition *d = new Definition();
			ParseDefinition(*d, word, definition, killIt);
			if(!killIt)
			{
				n->mDefinition[definitionType].push_back(d);

				#if DEBUG && 0
					TRACE("%s: %s\n", n->mWord.c_str(), d->mText.c_str());
				#endif
			}
			else
			{
				delete d;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////

void Purge()
{
	// remove all words not in the enable
	for(auto i = gWords.begin(); i != gWords.end(); )
	{
		Word *w = i->second;
		if(gEnable.find(w->mWord) == gEnable.end())
		{
			fprintf(stderr, "Removed NOENABLE %s\n", w->mWord.c_str());
			i = gWords.erase(i);
		}
		else
		{
			++i;
		}
	}

	for(auto i = gEnable.begin(); i != gEnable.end(); ++i)
	{
		if(gWords.find(*i) == gWords.end())
		{
			fprintf(stderr, "In ENABLE, not WIK %s\n", i->c_str());
		}
	}

	// mark as active all words with >=3 and <=7 letters
	for(auto i = gWords.begin(); i != gWords.end(); ++i)
	{
		Word *w = i->second;
		size_t l = w->mWord.length();
		w->mActive = l >= 3 && l <= 7;
	}

	// reactivate any words linked to by active words (eg XI, linked to by XIS) unless they're >7 long...
	for(auto it = gWords.begin(); it != gWords.end(); ++it)
	{
		Word *w = it->second;
		if(w->mActive)
		{
			for(int d = 0; d < Definition::kNumDefinitionTypes; ++d)
			{
				for(auto e = w->mDefinition[d].begin(); e != w->mDefinition[d].end(); ++e)
				{
					Definition &def = *(*e);

					if(def.mReference.mType != Reference::eType::kNone)
					{
						auto target = gWords.find(def.mReference.mReferenceWord);

						if(target != gWords.end() && target->second->mActive == false && target->second->mWord.size() <= 7)
						{
							target->second->mActive = true;
							fprintf(stderr, "including: %s\n", target->second->mWord.c_str());
						}
						else
						{
							fprintf(stderr, "Huh? Can't find %s\n", def.mReference.mReferenceWord.c_str());
						}
					}
				}
			}
		}
	}

	int erased;
	do
	{
		erased = 0;

		for(auto it = gWords.begin(); it != gWords.end(); ++it)
		{
			Word *w = it->second;
			for(int d = 0; d < Definition::kNumDefinitionTypes; ++d)
			{
				for(auto e = w->mDefinition[d].begin(); e != w->mDefinition[d].end(); )
				{
					Definition &def = *(*e);

					if(def.mReference.mType == Reference::eType::kNone)
					{
						string &str = def.mText;
						static regex rx("^\\((.*?)\\).*$");
						smatch m;

						if(regex_match(str, m, rx))
						{
							string &contents = m[1].str();

							static regex commaSplit(" *([^,]+)");

							sregex_iterator iter(contents.begin(), contents.end(), commaSplit);
							sregex_iterator ender;

							// some we want to kill the definition altogether

							static char const *killList[] =
							{
								"computing",
								"archaic",
								"MMORPG slang",
								"eye dialect",
								"eye dialect of",
								"Geordie",
								"vulgar"
							};

							// some we just want to remove

							static char const *removeList[] = 
							{
								"transitive",
								"intransitive",
								"uncountable",
								"countable"
							};

							bool killIt = false;

							string newDescription;

							for(; iter != ender && !killIt; ++iter)
							{
								bool removeIt = false;

								string &word = (*iter)[1].str();

								for(int i = 0; i < ARRAYSIZE(killList); ++i)
								{
									if(stricmp(killList[i], word.c_str()) == 0)
									{
										killIt = true;
										fprintf(stderr, "Killing [%s] because (slang, etc)\n", w->mWord.c_str());
										break;
									}
								}

								for(int i=0; i < ARRAYSIZE(removeList); ++i)
								{
									if(stricmp(removeList[i], word.c_str()) == 0)
									{
										removeIt = true;
										break;
									}
								}

								if(!removeIt)
								{
									if(newDescription.empty())
									{
										newDescription.append("(");
									}
									else
									{
										newDescription.append(", ");
									}
									newDescription.append(word);
								}
							}

							if(killIt)
							{
								e = w->mDefinition[d].erase(e);
								continue;
							}
							else
							{
								static regex stripper("^\\(.*?\\) ?(.+)$");
								def.mText = regex_replace(def.mText, stripper, "$1");

								if(!newDescription.empty())
								{
									def.mText = newDescription.append(") ").append(def.mText);
								}

							}
						}
					}
					++e;
				}
			}
		}

		// remove all words which are inactive or have no definitions

		for(auto it = gWords.begin(); it != gWords.end();)
		{
			if(it->second->GotAnyDefinitions() && it->second->mActive)
			{
				++it;
			}
			else
			{
				fprintf(stderr, "Removing [%s] because empty definition (1)\n", it->first);
				delete it->second;
				it = gWords.erase(it);
				erased++;
			}
		}

		// remove all definitions which point at words which either don't exist or don't have a definition of the right type

		for(auto i = gWords.begin(); i != gWords.end(); ++i)
		{
			Word *w = i->second;
			for(int d = 0; d < Definition::kNumDefinitionTypes; ++d)
			{
				for(auto e = w->mDefinition[d].begin(); e != w->mDefinition[d].end();)
				{
					Definition &def = *(*e);

					if(def.mReference.mType != Reference::eType::kNone)
					{
						// kill it because it's one of those that should be killed
						if(Reference::sKillTable[def.mReference.mType])
						{
							e = w->mDefinition[d].erase(e);
							continue;
						}

						// or because it points at a missing or inactive word
						auto target = gWords.find(def.mReference.mReferenceWord);

						if(target == gWords.end() || !target->second->mActive || !target->second->GotDefinition(d))
						{
							e = w->mDefinition[d].erase(e);
							continue;
						}

						auto p = gWords.find(def.mReference.mReferenceWord);
						if(p != gWords.end())
						{
							p->second->mActive = true;
						}
					}
					++e;
				}
			}
		}

		// remove all words which are inactive or have no definitions

		for(auto it = gWords.begin(); it != gWords.end();)
		{
			if(it->second->GotAnyDefinitions() && it->second->mActive)
			{
				++it;
			}
			else
			{
				fprintf(stderr, "Removing [%s] because empty definition (1)\n", it->first);
				delete it->second;
				it = gWords.erase(it);
				erased++;
			}
		}

		// now remove all @pointers@ that point at non-existent words

		for(auto it = gWords.begin(); it != gWords.end(); ++it)
		{
			Word *w = it->second;
			for(int d = 0; d < Definition::kNumDefinitionTypes; ++d)
			{
				for(auto e = w->mDefinition[d].begin(); e != w->mDefinition[d].end(); ++e)
				{
					Definition &def = *(*e);

					// scan for @word@, if it doesn't exist, kill the @'s

					static regex wordLink("@(.*?)@");

					string &str = def.mText;

					sregex_iterator a(str.begin(), str.end(), wordLink), b;

					string result;

					bool match = false;

					string finalstr;

					while(a != b)
					{
						match = true;

						string pre = a->prefix().str();
						string suf = a->suffix().str();
						string without = (*a)[1].str();
						string with = (*a)[0].str();

						result.append(a->prefix().str());

						auto f = gWords.find((*a)[1].str());

						if(f == gWords.end())
						{
							result.append((*a)[1].str());
						}
						else
						{
							result.append((*a)[0].str());
						}
						finalstr = a->suffix().str();

						++a;
					}

					result.append(finalstr);

					if(match)
					{
						if(!result.empty())
						{
							def.mText = result;
						}
					}
				}
			}
		}
	}
	while(erased != 0);
}

//////////////////////////////////////////////////////////////////////

struct CharMap
{
	//////////////////////////////////////////////////////////////////////

	map<uint32, int> mCharMap;

	//////////////////////////////////////////////////////////////////////

	void MapString(string const &str)
	{
		UTF8Decoder d((uint8 *)(str.c_str()), str.length());

		for(uint32 ch = d.Next(); ch != 0 && ch != 0xffffffff; ch = d.Next())
		{
			mCharMap[ch]++;
			ch = d.Next();
		}
	}

	//////////////////////////////////////////////////////////////////////

	wstring GetChars()
	{
		wstring s;
		for(auto c = mCharMap.begin(); c != mCharMap.end(); ++c)
		{
			s += (wchar)c->first;
		}
		return s;
	}

	//////////////////////////////////////////////////////////////////////

	string GetUTF8Chars()
	{
		wstring s = GetChars();
		string utf8;
		int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.length(), null, 0, null, null);
		utf8.resize(len);
		WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.length(), &utf8[0], (int)utf8.length(), null, null);
		return utf8;
	}
};


//////////////////////////////////////////////////////////////////////

void Output()
{
	CharMap charMap;

	int definitionsToInclude = 3;

	uint32 index = 0;
	for(auto i = gWords.begin(); i != gWords.end(); ++i)
	{
		Word *n = i->second;
		n->mIndex = index++;

		charMap.MapString(n->mWord);
		for(uint j=0; j<Definition::kNumDefinitionTypes; ++j)
		{
			if(!n->mDefinition[j].empty())
			{
				int numdefs = definitionsToInclude;
				for(auto k = n->mDefinition[j].begin(), e = n->mDefinition[j].end(); k != e; ++k)
				{
					Definition *d = *k;
					if(d->mReference.mType == Reference::eType::kNone)
					{
						charMap.MapString(d->mText);
					}
				}
			}
		}
	}

//	printf("// WordCount %d\n", gWords.size());
//	printf("// Characters: %s\n", charMap.GetUTF8Chars().c_str());
	printf("{\n");
	printf("\t\"words\": {\n");

	size_t wordCount = gWords.size();

	for(auto i = gWords.begin(); i != gWords.end(); ++i)
	{
		Word *n = i->second;
		printf("\t\t\"%s\": \"", n->mWord.c_str());

		for(uint j=0; j<Definition::kNumDefinitionTypes; ++j)
		{
			if(!n->mDefinition[j].empty())
			{
				// output up to N of each type
				int numdefs = definitionsToInclude;
				for(auto k = n->mDefinition[j].begin(), e = n->mDefinition[j].end(); k != e; ++k)
				{
					Definition *d = *k;
					printf("(%s) %s\\n", Definition::sDefinitionNames[j], d->mText.c_str());
					if(--numdefs == 0)
					{
						break;
					}
				}
			}
		}
		printf("\"%c\n", (--wordCount == 0) ? ' ' : ',');
	}
	printf("\t}\n}\n");

	// output a dictionary file

	//FILE *f = fopen("english.dictionary", "wb");
	//if(f != null)
	//{
	//	char *header = "dictionary";
	//	fwrite(header, sizeof(char), 10, f);
	//	
	//	char version[2] = { 1, 0 };
	//	fwrite(version, sizeof(char), 2, f);

	//	uint32 numWords = (uint32)gWords.size();
	//	fwrite(&numWords, sizeof(uint32), 1, f);

	//	// now write 5 bytes for each word

	//	for(auto i = gWords.begin(); i != gWords.end(); ++i)
	//	{
	//		uint8 word[8] = { 0 };
	//		uint8 *p = word;
	//		for(auto c = i->first.begin(); c != i->first.end(); ++c)
	//		{
	//			*p++ = *c;
	//		}
	//		fwrite(word, sizeof(uint8), 8, f);
	//	}

	//	// now write the definitions
	//	for(auto i = gWords.begin(); i != gWords.end(); ++i)
	//	{
	//		uint16 definitionMask = 0;
	//		for(int j = 0; j < Definition::kNumDefinitionTypes; ++j)
	//		{
	//			if(i->second->GotDefinition(j))
	//			{
	//				definitionMask |= (1 << j);
	//			}
	//		}
	//		fwrite(&definitionMask, sizeof(uint16), 1, f);
	//		for(int j = 0; j < Definition::kNumDefinitionTypes; ++j)
	//		{
	//			// 1st byte: reference type
	//			// 1st string: definition text or refword
	//			// 2nd string: only present if reference type is kCustom, in which case, it's the custom reference text
	//			if(i->second->GotDefinition(j))
	//			{
	//				Definition *d = i->second->mDefinition[j].front();
	//				uint8 type = (uint8)d->mReference.mType;
	//				if(type == Reference::eType::kNone)
	//				{
	//					fwrite(&type, sizeof(uint8), 1, f);
	//					string const &s = d->mText;
	//					fwrite(s.c_str(), sizeof(char), s.size() + 1, f);
	//				}
	//				else
	//				{
	//					// it's a reference
	//					// write the refword index as 3 bytes

	//					uint32 index = (gWords[d->mReference.mReferenceWord]->mIndex << 8) | type;
	//					fwrite(&index, sizeof(uint32), 1, f);
	//				}
	//			}
	//		}
	//	}

	//	fclose(f);
	//}
}

//////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
	LoadEnable();
	string s;
	while(getline(std::cin, s))
	{
		ProcessLine(s);
	}
	Purge();
	Output();
	return 0;
}

//////////////////////////////////////////////////////////////////////


