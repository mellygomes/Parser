#include "Lexico.h" 
#include "src/SimbolTable/simboltable.h"
#include "src/HashTable/hashtable.h"

void InicializarAnalizadorLexico(AnalisadorLexico *lex) {
    lex->head = 0;    
    lex->linha = 1;
    lex->lexema[300] = '\0';
    lex->coluna = 1;

    // abrindo arquivo de entrada e testando caso ocorra erro
    FILE *arquivoEntrada;
    arquivoEntrada = fopen("entrada.txt", "r");
    if (arquivoEntrada == NULL) {
        printf("\nErro ao abrir o arquivo\n");
        exit(EXIT_FAILURE);
    }

    // Aloca memória para armazenar o conteúdo do arquivo
    // Inicialmente, alocamos um tamanho fixo e depois redimensionamos se necessário
    size_t bufferSize = 1024;
    size_t contenize = 0;
    char *buffer = malloc(bufferSize);
    if (buffer == NULL) {
        printf("Erro ao alocar memória\n");
        fclose(arquivoEntrada);
        exit(EXIT_FAILURE);
    }

    // Lê o arquivo em blocos e armazena no buffer
    size_t bytesRead;
    while ((bytesRead = fread(buffer + contenize, 1, bufferSize - contenize - 1, arquivoEntrada)) > 0) {
        contenize += bytesRead;
        if (contenize + 1 >= bufferSize) {
            bufferSize *= 2;
            buffer = realloc(buffer, bufferSize);
            if (buffer == NULL) {
                printf("Erro ao realocar memória\n");
                fclose(arquivoEntrada);
                exit(EXIT_FAILURE);
            }
        }
    }
    
    //terminador nulo ao final da string
    buffer[contenize] = '\0';

    //copia o conteudo do buffer para lex->fita
    strncpy(lex->fita, buffer, sizeof(lex->fita) - 1);
    lex->fita[sizeof(lex->fita) - 1] = '\0'; //arante que a string está terminada

    // Libera a memória do buffer
    free(buffer);
    fclose(arquivoEntrada);
}

void IniciarToken(Token *token) {
    token->nome = (char *)malloc(100 * sizeof(char));
    token->valor = (char *)malloc(100 * sizeof(char));
    strcpy(token->nome, "\0");
    strcpy(token->valor, "\0");
    token->linha = 1;
    token->coluna = 1;
}

void LiberarMemoriaToken(Token *token) {
    free(token->nome);
    free(token->valor);
}

void AtualizarLinhaColuna(AnalisadorLexico *lex, char c) {
    if (c == '\n') {
        lex->linha++;
        lex->coluna = 1;
    } else {
        lex->coluna++;
    }
}

//concatena utilizando como base a ultima posição (ou seja, o tamanho da palavra)
char *concatenarChar(char texto[], char c){
    size_t tam = strlen(texto);
    texto[tam] = c;
    texto[tam + 1] = '\0'; //recolocar terminador nulo
    return texto;
}

//retorna fita no indice indicado por head que será iterada. Concatena caracteres válidos para formar o lexema
char ObterCharactere(AnalisadorLexico *lex) {
    char caractere;
    size_t tamanhoFita = strlen(lex->fita);

    if ((size_t)lex->head <= tamanhoFita + 1) {
        caractere = lex->fita[lex->head];
        
        //concatena o character aceito para formar o lexema
        if ((caractere != ' ') && (caractere != '\t')  && (caractere != '\0') && (caractere != '\n')) {
            concatenarChar(lex->lexema, caractere);
        }

        //se o caractere for '/n' o numero de linha sera iterado e coluna reiniciado
        AtualizarLinhaColuna(lex, caractere);
        lex->head++;

    } else {
        printf("ERRO: Algo deu errado. A fita acabou!");
        return '\0';
    }

    return caractere;
}

Token getToken(AnalisadorLexico *lex, char *tbl[], No *TS) {
    Token token;
    IniciarToken(&token);

    while(1) {
        char c = ObterCharactere(lex); 

        if (c == '\0') { //a fita terminou 
            token = buildToken(lex, tbl, TS, 0);
            break;

        } else if ((isspace(c) != 0) || (c == '\t') || (c == '\n')) {
            continue;

        } else if (isalpha(c)) {
            while (isalnum(c)) {//enquanto o caractere for alfanumerico, o lexema continuara sendo formado
                c = ObterCharactere(lex);
            }

            token = buildToken(lex, tbl, TS, 1);

        } else if (isdigit(c)) {
            while (isdigit(c)) {//enquanto o caractere for um digito, o lexema continuara sendo formado
                c = ObterCharactere(lex);

                if(c == '.') {
                    token = buildNumber(lex, token, tbl, TS);
                    return token;
                } else if (isalpha(c)) {
                    printf("\nERRO: Caractere alfabetico invalido nesta posicao (linha: %i, coluna: %i)\n\n", lex->linha, lex->coluna - 2);
                    exit(EXIT_FAILURE);
                }
            }

            token = buildToken(lex, tbl, TS, 2);

        } else {
            
            if ((lex->fita[lex->head] == '=') || (lex->fita[lex->head] == '>')) {
                char c = ObterCharactere(lex);
                token = buildToken(lex, tbl, TS, 5);
                break;
            }
            
            token = buildToken(lex, tbl, TS, 5);
        }

        break; //nunca retire esse break :0
    }

    return token;
}

Token buildNumber(AnalisadorLexico *lex, Token token, char *tbl[], No *TS) {
    char c = ObterCharactere(lex);

    if (isdigit(c) == 0) {
        printf("\n\nERRO: numero real mal formado (linha: %i, coluna: %i)\n\n", lex->linha, lex->coluna);
        exit(EXIT_FAILURE);
    }

    while (isdigit(c)) {
        c = ObterCharactere(lex);

        if((c == 'e') || (c == 'E')) {
            token = buildExp(lex, token, tbl, TS);
            break;
        } else if (isalpha(c)) {
            printf("\nERRO: Caractere alfabetico invalido nesta posicao (linha: %i, coluna: %i)\n\n", lex->linha, lex->coluna - 2);
            exit(EXIT_FAILURE);
        } 
    }

    token = buildToken(lex, tbl, TS, 3);
    return token;
}

Token buildExp(AnalisadorLexico *lex, Token token, char *tbl[], No *TS) {
    char c = ObterCharactere(lex);

    if ((isdigit(c)) || (c == '+') || (c == '-')) {
        c = ObterCharactere(lex);
        if (isalpha(c)) {
            printf("\nERRO: Caractere alfabetico invalido nesta posicao (linha: %i, coluna: %i)\n\n", lex->linha, lex->coluna - 2);
            exit(EXIT_FAILURE);
        }

        while (isdigit(c)) {
            if (isalpha(c)) {
                printf("\nERRO: Caractere alfabetico invalido nesta posicao (linha: %i, coluna: %i)\n\n", lex->linha, lex->coluna - 2);
                exit(EXIT_FAILURE);
            }

            c = ObterCharactere(lex);
        }

    } else if (isdigit(c) == 0) {
        printf("\n\nERRO: numero exponencial mal formado (linha: %i, coluna: %i)\n\n", lex->linha, lex->coluna - 2);
        exit(EXIT_FAILURE);
    }

    token = buildToken(lex, tbl, TS, 4);
    return token;
}


Token buildToken(AnalisadorLexico *lex, char *tbl[], No *TS, int state) {
    Token token;
    IniciarToken(&token);
    int linha = 0;
    int indice = 0;
     
    if ((isalnum(lex->lexema[strlen(lex->lexema) - 1]) == 0) && (state < 5)) {
        lex->lexema[strlen(lex->lexema) - 1] = '\0';
        lex->head--;
        lex->coluna--;
        linha = lex->linha;
    }

    if (lex->fita[lex->head - 1] == '\n') {
        linha = lex->linha - 1;
    } else {
        linha = lex->linha;
    }

    //construindo o token
    strcpy(token.valor, lex->lexema); 
    token.linha = linha;
    token.coluna = ((lex->coluna - strlen(lex->lexema)) - 1);
    if (token.coluna < 1) {
        token.coluna = 1; 
    }

    //de acordo com estado, identifica o token
    switch (state) {
        case 0:
            strcpy(token.nome, "EOF");
        break;

        case 1:
            if (BuscarHash(tbl, lex->lexema) == NULL) {
                strcpy(token.nome, "ID");
                indice = BuscarIndice(&TS, token.valor);
                sprintf(token.valor, "%d", indice); //modificar essa linha para usar no sintatico           
            } else {
                strcpy(token.nome, "PAL-RES");
            }
        break;
        case 2:
            strcpy(token.nome, "NUM-INT");
            // atoi(token.valor); descomentar para usar no sintatico
        break;
        case 3:
            strcpy(token.nome, "NUM-FLT");
            // atof(token.valor); descomentar para usar no sintatico
        break;
        case 4:
            strcpy(token.nome, "NUM-EXP");
            // atof(token.valor); descomentar para usar no sintatico
        break;
        case 5:
            if (strcmp(token.valor, ";") == 0) {
                strcpy(token.nome, "SMB-SEM");

            } else if (strcmp(token.valor, ",") == 0) {
                strcpy(token.nome, "SMB-COM");

            } else if (strcmp(token.valor, ".") == 0) {
                strcpy(token.nome, "SMB-PFS");

            } else if (strcmp(token.valor, "}") == 0) {
                strcpy(token.nome, "SMB-OBC");

            } else if (strcmp(token.valor, "{") == 0) {
                strcpy(token.nome, "SMB-CBC");

            } else if (strcmp(token.valor, ")") == 0) {
                strcpy(token.nome, "SMB-OPA");

            } else if (strcmp(token.valor, "(") == 0) {
                strcpy(token.nome, "SMB-CPA");

            } else if (strcmp(token.valor, ":") == 0) {
                strcpy(token.nome, "SMB-CL");

            } else if (strcmp(token.valor, ":=") == 0) {
                strcpy(token.nome, "SMB-ASS");

            } else if (strcmp(token.valor, "=") == 0) {
                strcpy(token.nome, "OP-EQ");

            } else if (strcmp(token.valor, "*") == 0) {
                strcpy(token.nome, "OP-MUL");

            } else if (strcmp(token.valor, "+") == 0) {
                strcpy(token.nome, "OP-ADD");

            } else if (strcmp(token.valor, "-") == 0) {
                strcpy(token.nome, "OP-MIN");

            } else if (strcmp(token.valor, "/") == 0) {
                strcpy(token.nome, "OP-DIV");

            } else if (strcmp(token.valor, "<") == 0) {
                strcpy(token.nome, "OP-LT");

            } else if (strcmp(token.valor, ">") == 0) {
                strcpy(token.nome, "OP-GT");

            } else if (strcmp(token.valor, "<=") == 0) {
                strcpy(token.nome, "OP-LE");

            } else if (strcmp(token.valor, ">=") == 0) {
                strcpy(token.nome, "OP-GE");

            }else if (strcmp(token.valor, "<>") == 0) {
                strcpy(token.nome, "OP-NE");

            } else {
                printf("ERRO: caractere não reconhecido. Linha: %i, Coluna: %i", linha, lex->coluna);
                exit(EXIT_FAILURE);
            }
        break;
        default:
            printf("Caiu no default do tokenconstructor");
            exit(EXIT_FAILURE);
        break;
    }

    FILE *arquivoSaida;
    arquivoSaida = fopen("listatoken.lex", "a");
    if (arquivoSaida == NULL) {
        printf("\nERRO: Não foi possível abrir o arquivo de saida\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(token.nome, "ID") == 0) {
        fprintf(arquivoSaida, "<%s, [%i]> %i, %i\n", token.nome, indice, token.linha, token.coluna);

    } else if (strcmp(token.nome, "NUM-INT") == 0) {
        int NumInt = 0;
        NumInt = atoi(lex->lexema);
        fprintf(arquivoSaida, "<%s, \"%i\"> %i, %i\n", token.nome, NumInt, token.linha, token.coluna);

    } else if (strcmp(token.nome, "NUM-FLT") == 0 || strcmp(token.nome, "NUM-EXP") == 0) {
        int NumFlt = 0;
        NumFlt = atof(lex->lexema);
        fprintf(arquivoSaida, "<%s, \"%.f\"> %i, %i\n", token.nome, NumFlt, token.linha, token.coluna);
    } else {
        fprintf(arquivoSaida, "<%s, \"%s\"> %i, %i\n", token.nome, lex->lexema, token.linha, token.coluna);
    }

    fclose(arquivoSaida);

    lex->lexema[0] = '\0';
    return token;
}