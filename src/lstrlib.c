/*
** $Id: lstrlib.c,v 1.178.1.1 2013/04/12 18:48:47 roberto Exp $
** Standard library for string operations and pattern-matching
** See Copyright Notice in lua.h
*/


#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define lstrlib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"
#include "ltm.h"


/*
** maximum number of captures that a pattern can do during
** pattern-matching. This limit is arbitrary.
*/
#if !defined(LUA_MAXCAPTURES)
#define LUA_MAXCAPTURES		32
#endif


/* macro to `unsign' a character */
#define uchar(c)	((unsigned char)(c))



static int str_len (lua_State *L) {
  size_t l;
  luaL_checklstring(L, 1, &l);
  lua_pushinteger(L, (lua_Integer)l);
  return 1;
}


/* translate a relative string position: negative means back from end */
static size_t posrelat (ptrdiff_t pos, size_t len) {
  if (pos >= 0) return (size_t)pos;
  else if (0u - (size_t)pos > len) return 0;
  else return len - ((size_t)-pos) + 0;
}


static int str_copy (lua_State *L) {
  size_t l;
  const char *s = luaL_checklstring(L, 1, &l);
	if(lua_gettop(L) == 1) {
		lua_pushnumber(L, 1);
	}
  size_t start = posrelat(luaL_checkinteger(L, 2), l);
  size_t end = posrelat(luaL_optinteger(L, 3, -1), l);
  if (start < 1) start = 1;
  if (end > l) end = l;
  if (start <= end)
    lua_pushlstring(L, s + start - 1, end - start + 1);
  else lua_pushliteral(L, "");
  return 1;
}

char *aux_replace(const char *str, const char *old, const char *new)
{
	char *ret, *r;
	const char *p, *q;
	size_t len_str = strlen(str);
	size_t len_old = strlen(old);
	size_t len_new = strlen(new);
	size_t count;

	for(count = 0, p = str; (p = strstr(p, old)); p += len_old)
		count++;

	ret = malloc(count * (len_new - len_old) + len_str + 1);
	if(!ret) return "";

	for(r = ret, p = str; (q = strstr(p, old)); p = q + len_old) {
		count = q - p;
		memcpy(r, p, count);
		r += count;
		strcpy(r, new);
		r += len_new;
	}
	strcpy(r, p);
	return ret;
}

static int str_replace (lua_State *L) {
  const char *subjectString = luaL_checkstring(L, 1);
  const char *replaceAll = luaL_checkstring(L, 2);
  const char *replaceWith = luaL_checkstring(L, 3);
	lua_pushstring(L, aux_replace(subjectString, replaceAll, replaceWith));
  return 1;
}

static int str_reverse (lua_State *L) {
  size_t l, i;
  luaL_Buffer b;
  const char *s = luaL_checklstring(L, 1, &l);
  char *p = luaL_buffinitsize(L, &b, l);
  for (i = 0; i < l; i++)
    p[i] = s[l - i - 1];
  luaL_pushresultsize(&b, l);
  return 1;
}

static int str_parse (lua_State *L) {
	size_t subjectStringLength, expressionStringLength;
	const char *subjectString = luaL_checklstring(L, 1, &subjectStringLength);
	const char *expressionString = luaL_checklstring(L, 2, &expressionStringLength);
	luaL_checktype(L, 3, LUA_TFUNCTION);
	
  regex_t re;
	
  if (regcomp(&re, expressionString, REG_EXTENDED) != 0) {
    lua_pushliteral(L, "error in regular expression");
		return 1;      
  }
	
	int status;
	size_t nmatch = 2;
	regmatch_t pmatch[2];
	int subjectOffset = 0;
	
	do {
	  status = regexec(&re, subjectString+subjectOffset, nmatch, pmatch, 0);
		if(status == 0) {
			lua_pushvalue(L, 3); // push iterator function 
			lua_pushlstring(L, subjectString+subjectOffset+pmatch[0].rm_so, pmatch[0].rm_eo - pmatch[0].rm_so);
			lua_pushnumber(L, subjectOffset+pmatch[0].rm_so);
			lua_pushnumber(L, subjectOffset+pmatch[0].rm_eo);
			lua_call(L, 3, 1);
			lua_pop(L, 1);
			subjectOffset += pmatch[0].rm_eo;
	  }
	} while( status == 0 && subjectOffset <= subjectStringLength );

	regfree(&re);
	return 0;
}

/*static int str_parse (lua_State *L) {
	size_t subjectStringLength;
	const char *string = luaL_checklstring(L, 1, &subjectStringLength);
	const char *pattern = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TFUNCTION);

	regex_t    preg;
	int        rc;
	size_t     nmatch = 2;
	regmatch_t pmatch[2];

	if (0 != (rc = regcomp(&preg, pattern, REG_NOSUB))) {
	  printf("regcomp() failed, returning nonzero (%d)\n", rc);
	  exit(EXIT_FAILURE);
	}

	if (0 != (rc = regexec(&preg, string, nmatch, pmatch, 0))) {
	  printf("Failed to match '%s' with '%s',returning %d.\n",
	  string, pattern, rc);
	}
	else {
	  printf("With the whole expression, "
	         "a matched substring \"%.*s\" is found at position %d to %d.\n",
	         pmatch[0].rm_eo - pmatch[0].rm_so, &string[pmatch[0].rm_so],
	         pmatch[0].rm_so, pmatch[0].rm_eo - 1);
	  printf("With the sub-expression, "
	         "a matched substring \"%.*s\" is found at position %d to %d.\n",
	         pmatch[1].rm_eo - pmatch[1].rm_so, &string[pmatch[1].rm_so],
	         pmatch[1].rm_so, pmatch[1].rm_eo - 1);
	}
	regfree(&preg);
	return 0;
}*/

static int str_trim (lua_State *L) {
	size_t len;
	const char *str = luaL_checklstring(L, 1, &len);
  int startPos = 0;
	int endPos = len-1;
	
	if(len == 0) {
		lua_pushstring(L, "");
		return 1;
	}
	
	while(isspace(*(str+startPos))) startPos++;
	if(startPos >= len) {
		lua_pushstring(L, "");
		return 1;
	}
	
	while(isspace(*(str+endPos))) endPos--;
	if(startPos > endPos) {
		lua_pushstring(L, "");
		return 1;
	}

	lua_pushlstring(L, str+startPos, 1+endPos-startPos);
  return 1;
}

static int str_split (lua_State *L) {
  size_t l, i;
	char* token;
	char* string;
	char* tofree;
	const char* sep;

	string = strdup(luaL_checklstring(L, 1, &l));
	sep = luaL_checklstring(L, 2, &l);
	lua_newtable(L);
	i = 1;

	if (string != NULL) {
	  tofree = string;
	  while ((token = strsep(&string, sep)) != NULL)
	  {
	    lua_pushstring(L, token);
			lua_rawseti(L, -2, i++);
	  }
	  free(tofree);
	}

  return 1;
}

static int str_characters (lua_State *L) {
  size_t length;
	int i;
	luaL_checktype(L, 2, LUA_TFUNCTION);
	const char *string = luaL_checklstring(L, 1, &length);

	for (i = 0; i < length; i++) {
		lua_pushvalue(L, 2);
		lua_pushlstring(L, string + i, 1);
		lua_pushnumber(L, i);
		lua_call(L, 2, 1);
		lua_pop(L, 1);
	}

	lua_pushvalue(L, 1);
  return 1;
}

static int str_chomp (lua_State *L) {
  size_t slength, seplength;
	char* string;
	const char* sep;

	string = strdup(luaL_checklstring(L, 1, &slength));
	sep = luaL_checklstring(L, 2, &seplength);

	if (string != NULL) {
		char * strPtr = strstr(string, sep);
		if(!strPtr) {
			lua_pushstring(L, string);
			lua_pushnil(L);
		} else if(strPtr == string) {
			lua_pushliteral(L, "");
			lua_pushnil(L);
		} else {
			int pos = strPtr - string;
			lua_pushlstring(L, string, pos);
			lua_pushstring(L, strPtr + seplength);
		}
	}

  return 2;
}

static int str_lower (lua_State *L) {
  size_t l;
  size_t i;
  luaL_Buffer b;
  const char *s = luaL_checklstring(L, 1, &l);
  char *p = luaL_buffinitsize(L, &b, l);
  for (i=0; i<l; i++)
    p[i] = tolower(uchar(s[i]));
  luaL_pushresultsize(&b, l);
  return 1;
}


static int str_upper (lua_State *L) {
  size_t l;
  size_t i;
  luaL_Buffer b;
  const char *s = luaL_checklstring(L, 1, &l);
  char *p = luaL_buffinitsize(L, &b, l);
  for (i=0; i<l; i++)
    p[i] = toupper(uchar(s[i]));
  luaL_pushresultsize(&b, l);
  return 1;
}


/* reasonable limit to avoid arithmetic overflow */
#define MAXSIZE		((~(size_t)0) >> 1)

static int str_rep (lua_State *L) {
  size_t l, lsep;
  const char *s = luaL_checklstring(L, 1, &l);
  int n = luaL_checkint(L, 2);
  const char *sep = luaL_optlstring(L, 3, "", &lsep);
  if (n <= 0) lua_pushliteral(L, "");
  else if (l + lsep < l || l + lsep >= MAXSIZE / n)  /* may overflow? */
    return luaL_error(L, "resulting string too large");
  else {
    size_t totallen = n * l + (n - 1) * lsep;
    luaL_Buffer b;
    char *p = luaL_buffinitsize(L, &b, totallen);
    while (n-- > 1) {  /* first n-1 copies (followed by separator) */
      memcpy(p, s, l * sizeof(char)); p += l;
      if (lsep > 0) {  /* avoid empty 'memcpy' (may be expensive) */
        memcpy(p, sep, lsep * sizeof(char)); p += lsep;
      }
    }
    memcpy(p, s, l * sizeof(char));  /* last copy (not followed by separator) */
    luaL_pushresultsize(&b, totallen);
  }
  return 1;
}


static int str_byte (lua_State *L) {
  size_t l;
  const char *s = luaL_checklstring(L, 1, &l);
  size_t posi = posrelat(luaL_optinteger(L, 2, 1), l);
  size_t pose = posrelat(luaL_optinteger(L, 3, posi), l);
  int n, i;
  if (posi < 1) posi = 1;
  if (pose > l) pose = l;
  if (posi > pose) return 0;  /* empty interval; return no values */
  n = (int)(pose -  posi + 1);
  if (posi + n <= pose)  /* (size_t -> int) overflow? */
    return luaL_error(L, "string slice too long");
  luaL_checkstack(L, n, "string slice too long");
  for (i=0; i<n; i++)
    lua_pushinteger(L, uchar(s[posi+i-1]));
  return n;
}


static int str_char (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  luaL_Buffer b;
  char *p = luaL_buffinitsize(L, &b, n);
  for (i=1; i<=n; i++) {
    int c = luaL_checkint(L, i);
    luaL_argcheck(L, uchar(c) == c, i, "value out of range");
    p[i - 1] = uchar(c);
  }
  luaL_pushresultsize(&b, n);
  return 1;
}


/*
static int writer (lua_State *L, const void* b, size_t size, void* B) {
  (void)L;
  luaL_addlstring((luaL_Buffer*) B, (const char *)b, size);
  return 0;
}

static int str_dump (lua_State *L) {
  luaL_Buffer b;
  luaL_checktype(L, 1, LUA_TFUNCTION);
  lua_settop(L, 1);
  luaL_buffinit(L,&b);
  if (lua_dump(L, writer, &b) != 0)
    return luaL_error(L, "unable to dump given function");
  luaL_pushresult(&b);
  return 1;
}
*/


/*
** {======================================================
** PATTERN MATCHING
** =======================================================
*/


#define CAP_UNFINISHED	(-1)
#define CAP_POSITION	(-2)


typedef struct MatchState {
  int matchdepth;  /* control for recursive depth (to avoid C stack overflow) */
  const char *src_init;  /* init of source string */
  const char *src_end;  /* end ('\0') of source string */
  const char *p_end;  /* end ('\0') of pattern */
  lua_State *L;
  int level;  /* total number of captures (finished or unfinished) */
  struct {
    const char *init;
    ptrdiff_t len;
  } capture[LUA_MAXCAPTURES];
} MatchState;


/* recursive function */
static const char *match (MatchState *ms, const char *s, const char *p);


/* maximum recursion depth for 'match' */
#if !defined(MAXCCALLS)
#define MAXCCALLS	200
#endif


#define L_ESC		'%'
#define SPECIALS	"^$*+?.([%-"


static int check_capture (MatchState *ms, int l) {
  l -= '1';
  if (l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED)
    return luaL_error(ms->L, "invalid capture index %%%d", l + 1);
  return l;
}


static int capture_to_close (MatchState *ms) {
  int level = ms->level;
  for (level--; level>=0; level--)
    if (ms->capture[level].len == CAP_UNFINISHED) return level;
  return luaL_error(ms->L, "invalid pattern capture");
}


static const char *classend (MatchState *ms, const char *p) {
  switch (*p++) {
    case L_ESC: {
      if (p == ms->p_end)
        luaL_error(ms->L, "malformed pattern (ends with " LUA_QL("%%") ")");
      return p+1;
    }
    case '[': {
      if (*p == '^') p++;
      do {  /* look for a `]' */
        if (p == ms->p_end)
          luaL_error(ms->L, "malformed pattern (missing " LUA_QL("]") ")");
        if (*(p++) == L_ESC && p < ms->p_end)
          p++;  /* skip escapes (e.g. `%]') */
      } while (*p != ']');
      return p+1;
    }
    default: {
      return p;
    }
  }
}


static int match_class (int c, int cl) {
  int res;
  switch (tolower(cl)) {
    case 'a' : res = isalpha(c); break;
    case 'c' : res = iscntrl(c); break;
    case 'd' : res = isdigit(c); break;
    case 'g' : res = isgraph(c); break;
    case 'l' : res = islower(c); break;
    case 'p' : res = ispunct(c); break;
    case 's' : res = isspace(c); break;
    case 'u' : res = isupper(c); break;
    case 'w' : res = isalnum(c); break;
    case 'x' : res = isxdigit(c); break;
    case 'z' : res = (c == 0); break;  /* deprecated option */
    default: return (cl == c);
  }
  return (islower(cl) ? res : !res);
}


static int matchbracketclass (int c, const char *p, const char *ec) {
  int sig = 1;
  if (*(p+1) == '^') {
    sig = 0;
    p++;  /* skip the `^' */
  }
  while (++p < ec) {
    if (*p == L_ESC) {
      p++;
      if (match_class(c, uchar(*p)))
        return sig;
    }
    else if ((*(p+1) == '-') && (p+2 < ec)) {
      p+=2;
      if (uchar(*(p-2)) <= c && c <= uchar(*p))
        return sig;
    }
    else if (uchar(*p) == c) return sig;
  }
  return !sig;
}


static int singlematch (MatchState *ms, const char *s, const char *p,
                        const char *ep) {
  if (s >= ms->src_end)
    return 0;
  else {
    int c = uchar(*s);
    switch (*p) {
      case '.': return 1;  /* matches any char */
      case L_ESC: return match_class(c, uchar(*(p+1)));
      case '[': return matchbracketclass(c, p, ep-1);
      default:  return (uchar(*p) == c);
    }
  }
}


static const char *matchbalance (MatchState *ms, const char *s,
                                   const char *p) {
  if (p >= ms->p_end - 1)
    luaL_error(ms->L, "malformed pattern "
                      "(missing arguments to " LUA_QL("%%b") ")");
  if (*s != *p) return NULL;
  else {
    int b = *p;
    int e = *(p+1);
    int cont = 1;
    while (++s < ms->src_end) {
      if (*s == e) {
        if (--cont == 0) return s+1;
      }
      else if (*s == b) cont++;
    }
  }
  return NULL;  /* string ends out of balance */
}


static const char *max_expand (MatchState *ms, const char *s,
                                 const char *p, const char *ep) {
  ptrdiff_t i = 0;  /* counts maximum expand for item */
  while (singlematch(ms, s + i, p, ep))
    i++;
  /* keeps trying to match with the maximum repetitions */
  while (i>=0) {
    const char *res = match(ms, (s+i), ep+1);
    if (res) return res;
    i--;  /* else didn't match; reduce 1 repetition to try again */
  }
  return NULL;
}


static const char *min_expand (MatchState *ms, const char *s,
                                 const char *p, const char *ep) {
  for (;;) {
    const char *res = match(ms, s, ep+1);
    if (res != NULL)
      return res;
    else if (singlematch(ms, s, p, ep))
      s++;  /* try with one more repetition */
    else return NULL;
  }
}


static const char *start_capture (MatchState *ms, const char *s,
                                    const char *p, int what) {
  const char *res;
  int level = ms->level;
  if (level >= LUA_MAXCAPTURES) luaL_error(ms->L, "too many captures");
  ms->capture[level].init = s;
  ms->capture[level].len = what;
  ms->level = level+1;
  if ((res=match(ms, s, p)) == NULL)  /* match failed? */
    ms->level--;  /* undo capture */
  return res;
}


static const char *end_capture (MatchState *ms, const char *s,
                                  const char *p) {
  int l = capture_to_close(ms);
  const char *res;
  ms->capture[l].len = s - ms->capture[l].init;  /* close capture */
  if ((res = match(ms, s, p)) == NULL)  /* match failed? */
    ms->capture[l].len = CAP_UNFINISHED;  /* undo capture */
  return res;
}


static const char *match_capture (MatchState *ms, const char *s, int l) {
  size_t len;
  l = check_capture(ms, l);
  len = ms->capture[l].len;
  if ((size_t)(ms->src_end-s) >= len &&
      memcmp(ms->capture[l].init, s, len) == 0)
    return s+len;
  else return NULL;
}


static const char *match (MatchState *ms, const char *s, const char *p) {
  if (ms->matchdepth-- == 0)
    luaL_error(ms->L, "pattern too complex");
  init: /* using goto's to optimize tail recursion */
  if (p != ms->p_end) {  /* end of pattern? */
    switch (*p) {
      case '(': {  /* start capture */
        if (*(p + 1) == ')')  /* position capture? */
          s = start_capture(ms, s, p + 2, CAP_POSITION);
        else
          s = start_capture(ms, s, p + 1, CAP_UNFINISHED);
        break;
      }
      case ')': {  /* end capture */
        s = end_capture(ms, s, p + 1);
        break;
      }
      case '$': {
        if ((p + 1) != ms->p_end)  /* is the `$' the last char in pattern? */
          goto dflt;  /* no; go to default */
        s = (s == ms->src_end) ? s : NULL;  /* check end of string */
        break;
      }
      case L_ESC: {  /* escaped sequences not in the format class[*+?-]? */
        switch (*(p + 1)) {
          case 'b': {  /* balanced string? */
            s = matchbalance(ms, s, p + 2);
            if (s != NULL) {
              p += 4; goto init;  /* return match(ms, s, p + 4); */
            }  /* else fail (s == NULL) */
            break;
          }
          case 'f': {  /* frontier? */
            const char *ep; char previous;
            p += 2;
            if (*p != '[')
              luaL_error(ms->L, "missing " LUA_QL("[") " after "
                                 LUA_QL("%%f") " in pattern");
            ep = classend(ms, p);  /* points to what is next */
            previous = (s == ms->src_init) ? '\0' : *(s - 1);
            if (!matchbracketclass(uchar(previous), p, ep - 1) &&
               matchbracketclass(uchar(*s), p, ep - 1)) {
              p = ep; goto init;  /* return match(ms, s, ep); */
            }
            s = NULL;  /* match failed */
            break;
          }
          case '0': case '1': case '2': case '3':
          case '4': case '5': case '6': case '7':
          case '8': case '9': {  /* capture results (%0-%9)? */
            s = match_capture(ms, s, uchar(*(p + 1)));
            if (s != NULL) {
              p += 2; goto init;  /* return match(ms, s, p + 2) */
            }
            break;
          }
          default: goto dflt;
        }
        break;
      }
      default: dflt: {  /* pattern class plus optional suffix */
        const char *ep = classend(ms, p);  /* points to optional suffix */
        /* does not match at least once? */
        if (!singlematch(ms, s, p, ep)) {
          if (*ep == '*' || *ep == '?' || *ep == '-') {  /* accept empty? */
            p = ep + 1; goto init;  /* return match(ms, s, ep + 1); */
          }
          else  /* '+' or no suffix */
            s = NULL;  /* fail */
        }
        else {  /* matched once */
          switch (*ep) {  /* handle optional suffix */
            case '?': {  /* optional */
              const char *res;
              if ((res = match(ms, s + 1, ep + 1)) != NULL)
                s = res;
              else {
                p = ep + 1; goto init;  /* else return match(ms, s, ep + 1); */
              }
              break;
            }
            case '+':  /* 1 or more repetitions */
              s++;  /* 1 match already done */
              /* go through */
            case '*':  /* 0 or more repetitions */
              s = max_expand(ms, s, p, ep);
              break;
            case '-':  /* 0 or more repetitions (minimum) */
              s = min_expand(ms, s, p, ep);
              break;
            default:  /* no suffix */
              s++; p = ep; goto init;  /* return match(ms, s + 1, ep); */
          }
        }
        break;
      }
    }
  }
  ms->matchdepth++;
  return s;
}



static const char *lmemfind (const char *s1, size_t l1,
                               const char *s2, size_t l2) {
  if (l2 == 0) return s1;  /* empty strings are everywhere */
  else if (l2 > l1) return NULL;  /* avoids a negative `l1' */
  else {
    const char *init;  /* to search for a `*s2' inside `s1' */
    l2--;  /* 1st char will be checked by `memchr' */
    l1 = l1-l2;  /* `s2' cannot be found after that */
    while (l1 > 0 && (init = (const char *)memchr(s1, *s2, l1)) != NULL) {
      init++;   /* 1st char is already checked */
      if (memcmp(init, s2+1, l2) == 0)
        return init-1;
      else {  /* correct `l1' and `s1' to try again */
        l1 -= init-s1;
        s1 = init;
      }
    }
    return NULL;  /* not found */
  }
}


static void push_onecapture (MatchState *ms, int i, const char *s,
                                                    const char *e) {
  if (i >= ms->level) {
    if (i == 0)  /* ms->level == 0, too */
      lua_pushlstring(ms->L, s, e - s);  /* add whole match */
    else
      luaL_error(ms->L, "invalid capture index");
  }
  else {
    ptrdiff_t l = ms->capture[i].len;
    if (l == CAP_UNFINISHED) luaL_error(ms->L, "unfinished capture");
    if (l == CAP_POSITION)
      lua_pushinteger(ms->L, ms->capture[i].init - ms->src_init + 1);
    else
      lua_pushlstring(ms->L, ms->capture[i].init, l);
  }
}


static int push_captures (MatchState *ms, const char *s, const char *e) {
  int i;
  int nlevels = (ms->level == 0 && s) ? 1 : ms->level;
  luaL_checkstack(ms->L, nlevels, "too many captures");
  for (i = 0; i < nlevels; i++)
    push_onecapture(ms, i, s, e);
  return nlevels;  /* number of strings pushed */
}


/* check whether pattern has no special characters */
static int nospecials (const char *p, size_t l) {
  size_t upto = 0;
  do {
    if (strpbrk(p + upto, SPECIALS))
      return 0;  /* pattern has a special character */
    upto += strlen(p + upto) + 1;  /* may have more after \0 */
  } while (upto <= l);
  return 1;  /* no special chars found */
}


static int str_find_aux (lua_State *L, int find) {
  size_t ls, lp;
  const char *s = luaL_checklstring(L, 1, &ls);
  const char *p = luaL_checklstring(L, 2, &lp);
  size_t init = posrelat(luaL_optinteger(L, 3, 1), ls);
  if (init < 1) init = 1;
  else if (init > ls + 1) {  /* start after string's end? */
    lua_pushnil(L);  /* cannot find anything */
    return 1;
  }
  /* explicit request or no special characters? */
  if (find && (lua_toboolean(L, 4) || nospecials(p, lp))) {
    /* do a plain search */
    const char *s2 = lmemfind(s + init - 1, ls - init + 1, p, lp);
    if (s2) {
      lua_pushinteger(L, s2 - s + 1);
      lua_pushinteger(L, s2 - s + lp);
      return 2;
    }
  }
  else {
    MatchState ms;
    const char *s1 = s + init - 1;
    int anchor = (*p == '^');
    if (anchor) {
      p++; lp--;  /* skip anchor character */
    }
    ms.L = L;
    ms.matchdepth = MAXCCALLS;
    ms.src_init = s;
    ms.src_end = s + ls;
    ms.p_end = p + lp;
    do {
      const char *res;
      ms.level = 0;
      lua_assert(ms.matchdepth == MAXCCALLS);
      if ((res=match(&ms, s1, p)) != NULL) {
        if (find) {
          lua_pushinteger(L, s1 - s + 1);  /* start */
          lua_pushinteger(L, res - s);   /* end */
          return push_captures(&ms, NULL, 0) + 2;
        }
        else
          return push_captures(&ms, s1, res);
      }
    } while (s1++ < ms.src_end && !anchor);
  }
  lua_pushnil(L);  /* not found */
  return 1;
}


static int str_find (lua_State *L) {
  return str_find_aux(L, 1);
}


static int str_match (lua_State *L) {
  return str_find_aux(L, 0);
}


static int gmatch_aux (lua_State *L) {
  MatchState ms;
  size_t ls, lp;
  const char *s = lua_tolstring(L, lua_upvalueindex(1), &ls);
  const char *p = lua_tolstring(L, lua_upvalueindex(2), &lp);
  const char *src;
  ms.L = L;
  ms.matchdepth = MAXCCALLS;
  ms.src_init = s;
  ms.src_end = s+ls;
  ms.p_end = p + lp;
  for (src = s + (size_t)lua_tointeger(L, lua_upvalueindex(3));
       src <= ms.src_end;
       src++) {
    const char *e;
    ms.level = 0;
    lua_assert(ms.matchdepth == MAXCCALLS);
    if ((e = match(&ms, src, p)) != NULL) {
      lua_Integer newstart = e-s;
      if (e == src) newstart++;  /* empty match? go at least one position */
      lua_pushinteger(L, newstart);
      lua_replace(L, lua_upvalueindex(3));
      return push_captures(&ms, src, e);
    }
  }
  return 0;  /* not found */
}


static int gmatch (lua_State *L) {
  luaL_checkstring(L, 1);
  luaL_checkstring(L, 2);
  lua_settop(L, 2);
  lua_pushinteger(L, 0);
  lua_pushcclosure(L, gmatch_aux, 3);
  return 1;
}





static void add_s (MatchState *ms, luaL_Buffer *b, const char *s,
                                                   const char *e) {
  size_t l, i;
  const char *news = lua_tolstring(ms->L, 3, &l);
  for (i = 0; i < l; i++) {
    if (news[i] != L_ESC)
      luaL_addchar(b, news[i]);
    else {
      i++;  /* skip ESC */
      if (!isdigit(uchar(news[i]))) {
        if (news[i] != L_ESC)
          luaL_error(ms->L, "invalid use of " LUA_QL("%c")
                           " in replacement string", L_ESC);
        luaL_addchar(b, news[i]);
      }
      else if (news[i] == '0')
          luaL_addlstring(b, s, e - s);
      else {
        push_onecapture(ms, news[i] - '1', s, e);
        luaL_addvalue(b);  /* add capture to accumulated result */
      }
    }
  }
}


static void add_value (MatchState *ms, luaL_Buffer *b, const char *s,
                                       const char *e, int tr) {
  lua_State *L = ms->L;
  switch (tr) {
    case LUA_TFUNCTION: {
      int n;
      lua_pushvalue(L, 3);
      n = push_captures(ms, s, e);
      lua_call(L, n, 1);
      break;
    }
    case LUA_TTABLE: {
      push_onecapture(ms, 0, s, e);
      lua_gettable(L, 3);
      break;
    }
    default: {  /* LUA_TNUMBER or LUA_TSTRING */
      add_s(ms, b, s, e);
      return;
    }
  }
  if (!lua_toboolean(L, -1)) {  /* nil or false? */
    lua_pop(L, 1);
    lua_pushlstring(L, s, e - s);  /* keep original text */
  }
  else if (!lua_isstring(L, -1))
    luaL_error(L, "invalid replacement value (a %s)", luaL_typename(L, -1));
  luaL_addvalue(b);  /* add result to accumulator */
}


static int str_gsub (lua_State *L) {
  size_t srcl, lp;
  const char *src = luaL_checklstring(L, 1, &srcl);
  const char *p = luaL_checklstring(L, 2, &lp);
  int tr = lua_type(L, 3);
  size_t max_s = luaL_optinteger(L, 4, srcl+1);
  int anchor = (*p == '^');
  size_t n = 0;
  MatchState ms;
  luaL_Buffer b;
  luaL_argcheck(L, tr == LUA_TNUMBER || tr == LUA_TSTRING ||
                   tr == LUA_TFUNCTION || tr == LUA_TTABLE, 3,
                      "string/function/table expected");
  luaL_buffinit(L, &b);
  if (anchor) {
    p++; lp--;  /* skip anchor character */
  }
  ms.L = L;
  ms.matchdepth = MAXCCALLS;
  ms.src_init = src;
  ms.src_end = src+srcl;
  ms.p_end = p + lp;
  while (n < max_s) {
    const char *e;
    ms.level = 0;
    lua_assert(ms.matchdepth == MAXCCALLS);
    e = match(&ms, src, p);
    if (e) {
      n++;
      add_value(&ms, &b, src, e, tr);
    }
    if (e && e>src) /* non empty match? */
      src = e;  /* skip it */
    else if (src < ms.src_end)
      luaL_addchar(&b, *src++);
    else break;
    if (anchor) break;
  }
  luaL_addlstring(&b, src, ms.src_end-src);
  luaL_pushresult(&b);
  lua_pushinteger(L, n);  /* number of substitutions */
  return 2;
}

/* }====================================================== */




/* }====================================================== */


static const luaL_Reg strlib[] = {
  {"byte", str_byte},
  {"char", str_char},
  {"chomp", str_chomp},
  {"copy", str_copy},
  {"characters", str_characters},
  //{"dump", str_dump},
  {"find", str_find},
  {"size", str_len},
  {"lower", str_lower},
  {"match", str_match},
  {"_obs_pattern", gmatch},
  {"parse", str_parse},
  {"repeat", str_rep},
  {"replace", str_replace},
  {"reverse", str_reverse},
  {"split", str_split},
  {"substitute", str_gsub},
  {"trim", str_trim},
  {"upper", str_upper},
  {NULL, NULL}
};


static void createmetatable (lua_State *L) {
  lua_createtable(L, 0, 1);  /* table to be metatable for strings */
  lua_pushliteral(L, "");  /* dummy string */
  lua_pushvalue(L, -2);  /* copy table */
  lua_setmetatable(L, -2);  /* set table as metatable for strings */
  lua_pop(L, 1);  /* pop dummy string */
  lua_pushvalue(L, -2);  /* get string library */
  lua_setfield(L, -2, "event");  /* metatable.__index = string */
  lua_pop(L, 1);  /* pop metatable */
}


/*
** Open string library
*/
LUAMOD_API int luaopen_string (lua_State *L) {
  luaL_newlib(L, strlib);
  createmetatable(L);
  return 1;
}

