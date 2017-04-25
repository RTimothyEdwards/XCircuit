/*--------------------------------------------------------------*/
/* pp.c --							*/
/*								*/
/* This helper program emulates m4 pre-processor behavior.	*/
/*								*/
/* As of July 2006, XCircuit no	longer uses m4, having replaced	*/
/* it with "sed" scripts.  However, since this C code was	*/
/* written to bypass the lack of "m4" under Windows, it is	*/
/* still required to bypass the lack of "sed" under Windows.	*/
/* The "m4" emulation is maintained.  However, the m4 files	*/
/* have been rewritten to remove the awkward "ifelse" syntax,	*/
/* which has been replaced with a simpler "<variable_name>"	*/
/* at the beginning of each variable-dependent line.  This	*/
/* program also handles those situations.			*/
/*--------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

typedef struct _pattern {
	char *pattern;
	char *string;
	struct _pattern *next;
} pattern;

pattern* parse_args(int* argc, char ***argv)
{
	int i, len = 0;
	pattern *p = (pattern*)malloc(sizeof(pattern));

	p->pattern = "`eval'";
	p->string = "eval";
	p->next = NULL;

	for (i=1; i<(*argc); i++) {
		if (strncmp((*argv)[i], "-D", 2) == 0) {
			char *c = strchr((*argv)[i], '=');
			pattern *new_p;
			if (c == NULL) {
				printf("Invalid argument: %s\n", (*argv[i]));
				exit(-1);
			}
			new_p = (pattern*)malloc(sizeof(pattern));
			new_p->next = p;
			p = new_p;
			c[0] = '\0';
			p->pattern = strdup((*argv)[i]+2);
			if (c[1] == '"') {
				char *c2;
				c += 2;
				c2 = strchr(c, '"');
				if (c2 != NULL)
					c2[0] = '\0';
			} else {
				char *c2;
				c++;
				c2 = strchr(c, ' ');
				if (c2 != NULL)
					c2[0] = '\0';
			}
			p->string = strdup(c);
			/*fprintf(stderr, "%s -> %s\n", p->pattern, p->string);*/
			len++;
		}
	}

	*argc -= len+1;
	*argv += len+1;

	return p;
}

int main(int argc, char **argv)
{
	char buffer[4096];
	char buffer2[4096];
	char *c;
	int i;
	pattern *patterns, *p;
	FILE *fin;

	patterns = parse_args(&argc, &argv);

	if (argc > 0) {
		fin = fopen(argv[0], "r");
		if (fin == NULL) {
			printf("Unable to open file: %s\n", argv[0]);
			exit(-1);
		}
	}
	else
		fin = stdin;

	while (1) {
		if (fgets(buffer, 4096, fin) == 0)
			exit(0);
		p = patterns;
		while (p) {
			while ((c = strstr(buffer, p->pattern)) != NULL) {
				c[0] = '\0';

				/* Handle variable-dependent lines */
			 	if (c == buffer && !strcmp(p->string, "1")) {
					strcpy(buffer2, c + strlen(p->pattern) + 1);
					strcpy(buffer, buffer2);
				}
				else if (c == buffer && !strcmp(p->string, "0")) {
					p = NULL;
					break;
				}
				else {
					strcpy(buffer2, buffer);
					strcat(buffer2, p->string);
					strcat(buffer2, c + strlen(p->pattern));
					strcpy(buffer, buffer2);
				}
			}
			if (p == NULL) break;
			p = p->next;
		}
		if (buffer[0] != '\0')
			printf("%s", buffer);
	}

	if (argc > 0)
		fclose(fin);

	return 0;
}
