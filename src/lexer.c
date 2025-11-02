#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "lexer.h"
#include "s8.h"
#include "util.h"

void lexer_init(Lexer* lexer, FILE* file, FILE* prompt) {
    lexer->file = file;
    lexer->prompt = prompt;

    lexer->input.ptr = NULL;
    lexer->input.len = 0;
    lexer->input_capacity = 0;

    lexer->start = 0;
    lexer->current = 0;

    lexer->line = 1;
    lexer->column = 1;
}

void lexer_init_s8(Lexer* lexer, s8 input) {
    lexer_init(lexer, NULL, NULL);

    lexer->input = input;
}

void lexer_free(Lexer* lexer) {
    lexer->file = NULL;
    lexer->prompt = NULL;

    if (lexer->input_capacity != 0) free(lexer->input.ptr);
    lexer->input.ptr = NULL;
    lexer->input.len = 0;
    lexer->input_capacity = 0;

    lexer->start = 0;
    lexer->current = 0;

    lexer->line = 1;
    lexer->column = 1;
}

static bool lexer_get_additional_input(Lexer* lexer) {
    // Input file isn't available, so we can't read additional input.
    if (lexer->file == NULL) return false;

    // The atypical sizing check is because `fgets()` requires more than 1
    // byte in the buffer, so we need to expand the buffer earlier.
    if (lexer->input.len + 1 >= lexer->input_capacity) {
        if (!GROW(&lexer->input.ptr, &lexer->input_capacity, 1, 128)) {
            fprintf(stderr, "failed to expand lexer buffer\n");
            exit(EXIT_FAILURE);
        }
    }

    if (lexer->prompt) {
        fprintf(lexer->prompt, "$> ");
        fflush(lexer->prompt);
    }

    uint8_t* additional_input_ptr = lexer->input.ptr + lexer->input.len;
    char* result = fgets(
        (char*) additional_input_ptr,
        lexer->input_capacity - lexer->input.len,
        lexer->file
    );

    if (result == NULL) {
        // No additional input returned, so we disable additional reads.
        lexer->file = NULL;
        return false;
    }

    size_t additional_len = strlen(result);
    if (additional_len == 0) {
        // No additional input returned, so we disable additional reads.
        lexer->file = NULL;
        return false;
    }

    lexer->input.len += additional_len;
    return true;
}

static bool is_at_end(Lexer* lexer) {
    if (lexer->current == lexer->input.len)
        return !lexer_get_additional_input(lexer);

    return false;
}

static uint8_t advance_byte(Lexer* lexer) {
    ASSERT(!is_at_end(lexer), "`advance_byte()` must not be called at the end");

    uint8_t byte = s8_index(lexer->input, lexer->current);
    lexer->current += 1;
    return byte;
}

static bool peek_nth_byte(Lexer* lexer, size_t nth, uint8_t* byte) {
    size_t read_index = lexer->current + nth;
    ASSERT(read_index >= lexer->current, "lexer peek index overflowed");

    while (read_index >= lexer->input.len) {
        // More input required.
        if (!lexer_get_additional_input(lexer)) {
            return false;
        }
    }

    *byte = s8_index(lexer->input, read_index);
    return true;
}

static bool peek_codepoint_at_offset(
    Lexer* lexer,
    bool* utf8_error,
    size_t* offset,
    uint32_t* codepoint
) {
    uint8_t byte;
    if (!peek_nth_byte(lexer, *offset, &byte)) goto handle_end;

    uint8_t width;
    uint32_t val;
    if (byte >> 7 == 0) {
        width = 1;
        val = byte & ~(1 << 7);
    } else if (byte >> 5 == 0x6) {
        width = 2;
        val = byte & ~(0x7 << 7);
    } else if (byte >> 4 == 0x16) {
        width = 3;
        val = byte & ~(0xF << 7);
    } else if (byte >> 3 == 0x36) {
        width = 4;
        val = byte & ~(0x1F << 7);
    } else {
        goto handle_utf8_error;
    }

    uint8_t byte_index = 1;
    while (byte_index < width) {
        if (!peek_nth_byte(lexer, *offset + byte_index, &byte)) {
            goto handle_utf8_error;
        }

        if (byte >> 6 != 0x2) {
            goto handle_utf8_error;
        }

        val = (val << 6) | (byte & ((1 << 6) - 1));
        byte_index += 1;
    }

    if (!(val <= 0x10FFFF) || (0xD800 <= val && val <= 0xDFFF)) {
        // `val` is either not in the unicode codepoint range or is a surrogate
        // code point, both of which invalid `val` for the purposes for this
        // lexer.
        goto handle_utf8_error;
    }

    *codepoint = val;
    *offset += width;
    return true;

handle_utf8_error:
    // Bytes located at index do not form a valid UTF-8 codepoint.
    *utf8_error = true;
    return false;

handle_end:
    // `index` is not a valid index into the input.
    *utf8_error = false;
    return false;
}

static bool advance(Lexer* lexer, bool* utf8_error, uint32_t* c) {
    size_t offset = 0;
    bool result = peek_codepoint_at_offset(lexer, utf8_error, &offset, c);
    if (result == true) lexer->current += offset;
    return result;
}

static uint32_t peek(Lexer* lexer, bool* utf8_error, uint32_t* c) {
    size_t offset = 0;
    return peek_codepoint_at_offset(lexer, utf8_error, &offset, c);
}

static uint32_t peek_next(Lexer* lexer, bool* utf8_error, uint32_t* c) {
    size_t offset = 0;
    if (!peek_codepoint_at_offset(lexer, utf8_error, &offset, c))
        return false;
    return peek_codepoint_at_offset(lexer, utf8_error, &offset, c);
}

static bool match(Lexer* lexer, uint32_t expected) {
    bool utf8_error = false;
    uint32_t c;

    if (!peek(lexer, &utf8_error, &c)) return false;
    if (c != expected) return false;
    return advance(lexer, &utf8_error, &c);
}

static void skip_non_token(Lexer* lexer) {
    bool utf8_error = false;
    uint32_t c;
    while (peek(lexer, &utf8_error, &c)) {
        switch (c) {
            case 0x20:
            case 0x0D:
            case 0x09:
                advance(lexer, &utf8_error, &c);
                break;
            case 0x0A:
                advance(lexer, &utf8_error, &c);
                lexer->line += 1;
                lexer->column = 1;
                break;
            case 0x2F:
                if (peek_next(lexer, &utf8_error, &c) && c == 0x2F) {
                    while (peek(lexer, &utf8_error, &c) != 0x0A)
                        advance(lexer, &utf8_error, &c);
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static bool is_digit(uint32_t c) {
    return 0x30 <= c && c <= 0x39;
}

static bool is_identifier_start(uint32_t c) {
    return (0x41 <= c && c <= 0x5A)
        || (0x61 <= c && c <= 0x7A)
        || c == 0x5F;
}

static bool is_identifier_continue(uint32_t c) {
    return is_identifier_start(c) || is_digit(c);
}

static void skip_invalid_utf8(Lexer* lexer) {
    bool utf8_error = false;
    uint32_t c;

    while (true) {
        if (peek(lexer, &utf8_error, &c))
            return;

        if (!utf8_error) {
            while (!is_at_end(lexer)) advance_byte(lexer);
            return;
        }
        advance_byte(lexer);
    }
}

static Token make_token(Lexer* lexer, TokenKind kind) {
    Token token;
    token.kind = kind;
    token.start = lexer->start;
    token.len = lexer->current - lexer->start;

    token.line = lexer->line;
    token.column = lexer->column;
    return token;
}

static Token make_utf8_error(Lexer* lexer) {
    skip_invalid_utf8(lexer);
    return make_token(lexer, TOKEN_UTF8_ERROR);
}

static Token character(Lexer* lexer) {
    bool contains_utf8_error = false;

    bool utf8_error = false;
    uint32_t c;

    while (1) {
        while (peek(lexer, &utf8_error, &c)) {
            if (c == 0x27) goto finish_character;
            if (c == 0x0A) {
                lexer->line += 1;
                lexer->column = 1;
            }
            advance(lexer, &utf8_error, &c);
        }

        if (utf8_error) {
            contains_utf8_error = true;
            skip_invalid_utf8(lexer);
        } else {
            return make_token(lexer, TOKEN_UNTERMINATED_CHAR_ERROR);
        }
    }
finish_character:
    advance(lexer, &utf8_error, &c);
    return make_token(
        lexer,
        contains_utf8_error ? TOKEN_UTF8_ERROR_IN_CHAR : TOKEN_CHAR
    );
}

static Token number(Lexer* lexer) {
    bool contains_utf8_error = false;

    bool utf8_error = false;
    uint32_t c;

    while (1) {
        while (peek(lexer, &utf8_error, &c)) {
            if (!is_digit(c)) goto check_dot;
            advance(lexer, &utf8_error, &c);
        }

        if (utf8_error) {
            contains_utf8_error = true;
            skip_invalid_utf8(lexer);
        }
    }
check_dot:
    if (c == 0x2E && peek_next(lexer, &utf8_error, &c) && is_digit(c)) {
        advance(lexer, &utf8_error, &c); // Consume the dot.

        while (1) {
            while (peek(lexer, &utf8_error, &c)) {
                if (!is_digit(c)) goto finish_number;
                advance(lexer, &utf8_error, &c);
            }

            if (utf8_error) {
                contains_utf8_error = true;
                skip_invalid_utf8(lexer);
            }
        }
    }
finish_number:
    return make_token(
        lexer,
        contains_utf8_error ? TOKEN_UTF8_ERROR_IN_NUMBER : TOKEN_NUMBER
    );
}

static Token string(Lexer* lexer) {
    bool contains_utf8_error = false;

    bool utf8_error = false;
    uint32_t c;

    while (1) {
        while (peek(lexer, &utf8_error, &c)) {
            if (c == 0x22) goto finish_string;
            if (c == 0x0A) {
                lexer->line += 1;
                lexer->column = 1;
            }
            advance(lexer, &utf8_error, &c);
        }

        if (utf8_error) {
            contains_utf8_error = true;
            skip_invalid_utf8(lexer);
        } else {
            return make_token(lexer, TOKEN_UNTERMINATED_STRING_ERROR);
        }
    }
finish_string:
    advance(lexer, &utf8_error, &c);
    return make_token(
        lexer,
        contains_utf8_error ? TOKEN_UTF8_ERROR_IN_STRING : TOKEN_STRING
    );
}

static TokenKind check_keyword(
    s8 identifier,
    size_t start,
    uint8_t* bytes,
    size_t byte_count,
    TokenKind kind
) {
    ASSERT(start <= identifier.len);

    identifier = (s8) { identifier.ptr + start, identifier.len - start };
    if (identifier.len != byte_count) return TOKEN_IDENTIFIER;
    if (memcmp(identifier.ptr, bytes, sizeof(uint8_t) * byte_count) == 0)
        return kind;

    return TOKEN_IDENTIFIER;
}

static TokenKind identifier_kind(Lexer* lexer) {
    s8 identifier = (s8) {
        lexer->input.ptr + lexer->start,
        lexer->current - lexer->start
    };

    switch (s8_index(identifier, 0)) {
        case 0x61: return check_keyword(
            identifier,
            1,
            (uint8_t[]){ 0x73 },
            1,
            TOKEN_AS
        );
        case 0x63: return check_keyword(
            identifier,
            1,
            (uint8_t[]){ 0x6F, 0x6E, 0x73, 0x74 },
            4,
            TOKEN_CONST
        );
        case 0x65:
            if (identifier.len <= 1) break;
            switch (s8_index(identifier, 1)) {
                case 0x6C: return check_keyword(
                    identifier,
                    2,
                    (uint8_t[]){ 0x73, 0x65 },
                    2,
                    TOKEN_ELSE
                );
                case 0x6E: return check_keyword(
                    identifier,
                    2,
                    (uint8_t[]){ 0x75, 0x6D },
                    2,
                    TOKEN_ENUM
                );
            }
            break;
        case 0x66:
            if (identifier.len <= 1) break;
            switch (s8_index(identifier, 1)) {
                case 0x61: return check_keyword(
                    identifier,
                    2,
                    (uint8_t[]){ 0x6C, 0x73, 0x65 },
                    3,
                    TOKEN_FALSE
                );
                case 0x6E: return check_keyword(
                    identifier,
                    2,
                    (uint8_t[]){ },
                    0,
                    TOKEN_FN
                );
            }
            break;
        case 0x69:
            if (identifier.len <= 1) break;
            switch (s8_index(identifier, 1)) {
                case 0x66: return check_keyword(
                    identifier,
                    2,
                    (uint8_t[]){ },
                    0,
                    TOKEN_IF
                );
                case 0x6D: return check_keyword(
                    identifier,
                    2,
                    (uint8_t[]){ 0x70, 0x6C },
                    2,
                    TOKEN_IMPL
                );
            }
            break;
        case 0x6C: return check_keyword(
            identifier,
            1,
            (uint8_t[]){ 0x65, 0x74 },
            2,
            TOKEN_LET
        );
        case 0x72: return check_keyword(
            identifier,
            1,
            (uint8_t[]){ 0x65, 0x74, 0x75, 0x72, 0x6E },
            5,
            TOKEN_RETURN
        );
        case 0x73:
            if (identifier.len <= 2) break;
            if (s8_index(identifier, 1) != 0x74) break;
            switch (s8_index(identifier, 2)) {
                case 0x61: return check_keyword(
                    identifier,
                    3,
                    (uint8_t[]){ 0x74, 0x69, 0x63 },
                    3,
                    TOKEN_STATIC
                );
                case 0x72: return check_keyword(
                    identifier,
                    3,
                    (uint8_t[]){ 0x75, 0x63, 0x74 },
                    3,
                    TOKEN_STRUCT
                );
            }
            break;
        case 0x74: return check_keyword(
            identifier,
            1,
            (uint8_t[]){ 0x72, 0x75, 0x65 },
            3,
            TOKEN_TRUE
        );
        case 0x77: return check_keyword(
            identifier,
            1,
            (uint8_t[]){ 0x68, 0x69, 0x6C, 0x65 },
            4,
            TOKEN_WHILE
        );
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier(Lexer* lexer) {
    bool utf8_error = false;
    uint32_t c;

    while (peek(lexer, &utf8_error, &c)) {
        if (!is_identifier_continue(c)) break;
        advance(lexer, &utf8_error, &c);
    }

    return make_token(lexer, identifier_kind(lexer));
}

Token lexer_next_token(Lexer* lexer) {
    skip_non_token(lexer);
    lexer->start = lexer->current;

    bool utf8_error = false;
    uint32_t c;
    if (!advance(lexer, &utf8_error, &c)) return make_token(lexer, TOKEN_EOF);
    if (utf8_error) return make_utf8_error(lexer);

    if (is_identifier_start(c)) return identifier(lexer);
    if (is_digit(c)) return number(lexer);
    switch (c) {
        // Single character tokens.
        case 0x2C: return make_token(lexer, TOKEN_COMMA);
        case 0x2E: return make_token(lexer, TOKEN_DOT);
        case 0x7B: return make_token(lexer, TOKEN_LEFT_BRACE);
        case 0x7D: return make_token(lexer, TOKEN_RIGHT_BRACE);
        case 0x40: return make_token(lexer, TOKEN_LEFT_PAREN);
        case 0x41: return make_token(lexer, TOKEN_RIGHT_PAREN);
        case 0x5B: return make_token(lexer, TOKEN_LEFT_SQUARE_BRACKET);
        case 0x5D: return make_token(lexer, TOKEN_RIGHT_SQUARE_BRACKET);
        case 0x3B: return make_token(lexer, TOKEN_SEMICOLON);

        // Single or double character tokens.
        case 0x26:
            if (match(lexer, 0x26)) return make_token(lexer, TOKEN_AMPERSAND_AMPERSAND);
            if (match(lexer, 0x3D)) return make_token(lexer, TOKEN_AMPERSAND_EQUAL);
            return make_token(lexer, TOKEN_AMPERSAND);
        case 0x21:
            if (match(lexer, 0x3D)) return make_token(lexer, TOKEN_BANG_EQUAL);
            return make_token(lexer, TOKEN_BANG);
        case 0x5E:
            if (match(lexer, 0x5E)) return make_token(lexer, TOKEN_CARET_CARET);
            if (match(lexer, 0x3D)) return make_token(lexer, TOKEN_CARET_EQUAL);
            return make_token(lexer, TOKEN_CARET);
        case 0x3A:
            if (match(lexer, 0x3A)) return make_token(lexer, TOKEN_COLON_COLON);
            return make_token(lexer, TOKEN_COLON);
        case 0x3D:
            if (match(lexer, 0x3D)) return make_token(lexer, TOKEN_EQUAL_EQUAL);
            return make_token(lexer, TOKEN_EQUAL);
        case 0x3E:
            if (match(lexer, 0x3D)) return make_token(lexer, TOKEN_GREATER_EQUAL);
            return make_token(lexer, TOKEN_GREATER);
        case 0x3C:
            if (match(lexer, 0x3D)) return make_token(lexer, TOKEN_LESS_EQUAL);
            return make_token(lexer, TOKEN_LESS);
        case 0x2D:
            if (match(lexer, 0x3D)) return make_token(lexer, TOKEN_MINUS_EQUAL);
            if (match(lexer, 0x3E)) return make_token(lexer, TOKEN_MINUS_GREATER);
            return make_token(lexer, TOKEN_MINUS);
        case 0x2B:
            if (match(lexer, 0x3D)) return make_token(lexer, TOKEN_PLUS_EQUAL);
            return make_token(lexer, TOKEN_PLUS);
        case 0x2A:
            if (match(lexer, 0x3D)) return make_token(lexer, TOKEN_STAR_EQUAL);
            return make_token(lexer, TOKEN_STAR);
        case 0x2F:
            if (match(lexer, 0x3D)) return make_token(lexer, TOKEN_SLASH_EQUAL);
            return make_token(lexer, TOKEN_SLASH);
        case 0x7C:
            if (match(lexer, 0x7C)) return make_token(lexer, TOKEN_PIPE_PIPE);
            return make_token(lexer, TOKEN_PIPE);
        case 0x25:
            if (match(lexer, 0x3D)) return make_token(lexer, TOKEN_PERCENT_EQUAL);
            return make_token(lexer, TOKEN_PERCENT);

        // Literal parsing.
        case 0x27: return character(lexer);
        case 0x22: return string(lexer);
        default:
            return make_token(lexer, TOKEN_UNEXPECTED_CHARACTER);
    }
}
