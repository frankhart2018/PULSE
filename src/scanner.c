#include <stdio.h>
#include <string.h>

#include "../includes/common.h"
#include "../includes/scanner.h"

void initScanner(Scanner* scanner, const char* source) {
  scanner->start = source;
  scanner->current = source;
  scanner->line = 1;
  scanner->indentLevel = 0;
  scanner->isIndent = false;
  scanner->isUnindent = false;
  scanner->unindentLevel = 0;
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_';
}

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool isAtEnd(Scanner* scanner) {
  return *scanner->current == '\0';
}

static char advance(Scanner* scanner) {
  scanner->current++;
  return scanner->current[-1];
}

static char peek(Scanner* scanner) {
  return *scanner->current;
}

static char peekNext(Scanner* scanner) {
  if(isAtEnd(scanner)) return '\0';
  return scanner->current[1];
}

static char peekNextZeroth(Scanner* scanner) {
  if(isAtEnd(scanner)) return '\0';
  return scanner->current[0];
}

static char peekNextLine(Scanner* scanner) {
  if(isAtEnd(scanner)) return '\0';
  int i = 0;
  while(scanner->current[i] != '\n') ++i;
  ++i;
  return scanner->current[i];
}

static bool match(Scanner* scanner, char expected) {
  if(isAtEnd(scanner)) return false;
  if(*scanner->current != expected) return false;

  scanner->current++;
  return true;
}

static Token makeToken(Scanner* scanner, TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner->start;
  token.length = (int)(scanner->current - scanner->start);
  token.line = scanner->line;

  return token;
}

static Token errorToken(Scanner* scanner, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner->line;

    return token;
}

static void skipWhitespace(Scanner* scanner) {
  for( ; ; ) {
    char c = peek(scanner);
    switch(c) {
      case ' ':
      case '\r':
        advance(scanner);
        break;

        case '#':
          while(peek(scanner) != '\n' && !isAtEnd(scanner))
            advance(scanner);
          if(!isAtEnd(scanner))
            advance(scanner);
          scanner->line++;
          break;

        case '~':
          while(peek(scanner) != '\n' && !isAtEnd(scanner))
            advance(scanner);
          if(!isAtEnd(scanner))
            advance(scanner);
          scanner->line++;
          break;

        case '/':
          if(peekNext(scanner) == '*') {
            advance(scanner);
            advance(scanner);
            while(peek(scanner) != '*' && !isAtEnd(scanner)) {
              if(peek(scanner) == '\n')
                scanner->line++;
              advance(scanner);
            }
            if(peekNext(scanner) == '/') {
              scanner->line++;
              advance(scanner);
              advance(scanner);
              advance(scanner);
              break;
            } else {
              return;
            }
          } else {
            return;
          }
          break;

      default:
        return;
    }
  }
}

static TokenType checkKeyword(Scanner* scanner, int start, int length, const char* rest, TokenType type) {
  if(scanner->current - scanner->start == start + length && memcmp(scanner->start + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static TokenType identifierType(Scanner* scanner) {
  switch(scanner->start[0]) {
    case 'a': return checkKeyword(scanner, 1, 2, "nd", TOKEN_AND);
    case 'c': return checkKeyword(scanner, 1, 4, "lass", TOKEN_CLASS);
    case 'd': return checkKeyword(scanner, 1, 2, "ef", TOKEN_DEF);
    case 'e': return checkKeyword(scanner, 1, 3, "lse", TOKEN_ELSE);
    case 'f': {
      if (scanner->current - scanner->start > 1) {
        switch (scanner->start[1]) {
          case 'a': return checkKeyword(scanner, 2, 3, "lse", TOKEN_FALSE);
          case 'o': return checkKeyword(scanner, 2, 1, "r", TOKEN_FOR);
        }
      }
      break;
		}
    case 'i': {
			if(scanner->current - scanner->start > 1) {
				switch(scanner->start[1]) {
					case 'n': return checkKeyword(scanner, 2, 3, "put", TOKEN_INPUT);
					case 'f': return checkKeyword(scanner, 1, 1, "f", TOKEN_IF);
				}
			}
		}
    case 'n': return checkKeyword(scanner, 1, 2, "il", TOKEN_NIL);
    case 'o': return checkKeyword(scanner, 1, 1, "r", TOKEN_OR);
    case 'p': return checkKeyword(scanner, 1, 4, "rint", TOKEN_PRINT);
    case 'r': return checkKeyword(scanner, 1, 5, "eturn", TOKEN_RETURN);
    case 's': return checkKeyword(scanner, 1, 4, "uper", TOKEN_SUPER);
    case 't':
      if (scanner->current - scanner->start > 1) {
        switch (scanner->start[1]) {
          case 'h': return checkKeyword(scanner, 2, 2, "is", TOKEN_THIS);
          case 'r': return checkKeyword(scanner, 2, 2, "ue", TOKEN_TRUE);
        }
      }
      break;
    case 'v': return checkKeyword(scanner, 1, 2, "ar", TOKEN_VAR);
    case 'w': return checkKeyword(scanner, 1, 4, "hile", TOKEN_WHILE);
  }

  return TOKEN_IDENTIFIER;
}

static Token identifier(Scanner* scanner) {
  while(isAlpha(peek(scanner)) || isDigit(peek(scanner))) advance(scanner);

  return makeToken(scanner, identifierType(scanner));
}

static Token number(Scanner* scanner) {
  while(isDigit(peek(scanner))) advance(scanner);

  if(peek(scanner) == '.' && isDigit(peekNext(scanner))) {
    advance(scanner);

    while(isDigit(peek(scanner))) advance(scanner);
  }

  return makeToken(scanner, TOKEN_NUMBER);
}

static Token string(Scanner* scanner) {
  while(peek(scanner) != '"' && !isAtEnd(scanner)) {
    if(peek(scanner) == '\n') scanner->line++;
    advance(scanner);
  }

  if(isAtEnd(scanner)) return errorToken(scanner, "Unterminated string.");

  advance(scanner);
  return makeToken(scanner, TOKEN_STRING);
}

static void checkUnindent(Scanner* scanner) {
  if(scanner->isIndent) {
    int localTabCount = 0;
    while(peekNextZeroth(scanner) == '\t') {
      localTabCount++;
      advance(scanner);
    }
    if(peek(scanner) == '\t') {
      advance(scanner);
    }
    if(localTabCount < scanner->indentLevel) {
      scanner->isUnindent = true;
      scanner->unindentLevel = scanner->indentLevel - localTabCount - 1;
      if(localTabCount == 0)
        scanner->indentLevel = 0;
      else
        scanner->indentLevel--;

      if(scanner->unindentLevel > 0 && localTabCount != 0) {
        if(scanner->unindentLevel - 1 > 0 && peekNextLine(scanner) != '\0')
          scanner->indentLevel = scanner->unindentLevel - 1;
        else
          scanner->indentLevel = localTabCount;
      }

    }
    if(scanner->indentLevel == 0) {
      scanner->isIndent = false;
    }
  }
}

Token scanToken(Scanner* scanner) {
  if(scanner->isUnindent) {
    if(scanner->unindentLevel == 0)
      scanner->isUnindent = false;
    else
      scanner->unindentLevel--;
    return makeToken(scanner, TOKEN_UNINDENT);
  }

  skipWhitespace(scanner);

  scanner->start = scanner->current;

  if(isAtEnd(scanner)) return makeToken(scanner, TOKEN_EOF);

  char c = advance(scanner);

  if(isAlpha(c)) return identifier(scanner);
  if(isDigit(c)) return number(scanner);

  switch(c) {
    case '(': return makeToken(scanner, TOKEN_LEFT_PAREN);
    case ')': return makeToken(scanner, TOKEN_RIGHT_PAREN);
    case '{': return makeToken(scanner, TOKEN_LEFT_BRACE);
    case '}': return makeToken(scanner, TOKEN_RIGHT_BRACE);
    case ':': {
      scanner->isIndent = true;
      scanner->indentLevel++;
      scanner->line++;
      if(peekNextZeroth(scanner) == '\n') {
        advance(scanner);
        checkUnindent(scanner);
      }
      return makeToken(scanner, TOKEN_BEGIN_BLOCK);
    }
    case '\n': {
      scanner->line++;
      checkUnindent(scanner);
      return makeToken(scanner, TOKEN_NEWLINE);
    }
    case ';': return makeToken(scanner, TOKEN_SEMICOLON);
    case ',': return makeToken(scanner, TOKEN_COMMA);
    case '.': return makeToken(scanner, TOKEN_DOT);
    case '-': return makeToken(scanner, TOKEN_MINUS);
    case '+': return makeToken(scanner, TOKEN_PLUS);
    case '/':  {
        if(peek(scanner) == '/') {
          advance(scanner);
          return makeToken(scanner, TOKEN_SLASH_INT);
        }
        return makeToken(scanner, TOKEN_SLASH);
    }
    case '*': return makeToken(scanner, TOKEN_STAR);
    case '%': return makeToken(scanner, TOKEN_MODULO);
    case '^': return makeToken(scanner, TOKEN_POWER);
    case '!': return makeToken(scanner, match(scanner, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=': return makeToken(scanner, match(scanner, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<': return makeToken(scanner, match(scanner, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>': return makeToken(scanner, match(scanner, '=') ? TOKEN_GREATER_EQUAL: TOKEN_GREATER);
    case '"': return string(scanner);
  }
  return errorToken(scanner, "Unexpected character.");
}
