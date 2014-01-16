


#include <stdlib.h>
#include <assert.h>
#include "generator.h"

#define N_MACRO_IMPLEMENT
#include "grammar.h"
#define FOREACH(val,coll) for(__typeof__((coll).elem[0]) *val=(coll).elem;val<(coll).elem + (coll).count;val++)
#define p_format "%.*s"
#define p_arg(x) x.count,x.elem
typedef struct rope{
  struct rope *prev;
  const char *str;
  size_t len;
} rope;
struct rope *rope_new(const char *cstr,size_t length, struct rope *prev){
        struct rope *r = malloc(sizeof(rope));
        r->prev = prev;
        r->str = cstr;
        r->len = length;
        return r;
}
struct rope *rope_const (const char*cstr, struct rope *prev ){
        return rope_new(cstr,strlen(cstr),prev);
}
#define rope_narray(array,prev) rope_new((array).elem,(array).count,prev)
void rope_print(struct rope *r){
        if(r->prev)
                rope_print(r->prev);
        printf("%.*s",r->len,r->str);
}

void write_parser(parser_invocation *p){
        printf("write_%.*s;",p->name.count,p->name.elem);
}
void write_parser_bind(parser_invocation *p,rope *value){
        printf("write_%.*s(",p->name.count,p->name.elem);
        rope_print(value);
        printf(")\n");
}
void emit_parserrule(parserrule *rule,rope *value);
/*ABI: write_bits, save_bits, write_bits_at*/
void emit_field(struct_field *field,rope *value)
{
        emit_parserrule(&field->contents,rope_narray(field->name,rope_const("->",value)));
} 
void emit_const(struct_const *field)
{
        write_parser(&field->contents); 
}
void emit_STRUCT(struct_rule *rule,rope *value){
        FOREACH(struct_elem,rule->fields)
        {
                switch(struct_elem->N_type){
                case S_FIELD:
                        emit_field(&struct_elem->S_FIELD,value);
                        break;
                case S_CONST:
                        emit_const(&struct_elem->S_CONST);
                        break;
                }
        }
}
void emit_ARRAY(array_rule *array,rope *value){
//TODO: check for many1 - emit check!. Check for other values too!
        if(strncmp("h_many1",array->parser_combinator.elem,array->parser_combinator.count)){
                printf("if(0==");
                rope_print(rope_const(".count)",value));
                printf("{return NULL;}\n");
        }
        printf("FOREACH(iter,");
        rope_print(value);
        printf("){\n");
        emit_parserrule(&array->contents,rope_const("iter",NULL));
        printf("}\n");
}
void emit_REF(ref_rule *ref,rope *value){
        printf("emit_%.*s(*(", ref->name.count, ref->name.elem);
        rope_print(value);
        printf("));");        
}
void emit_EMBED(embed_rule *embed,rope *value){
        printf("emit_%.*s(", embed->name.count, embed->name.elem);
        rope_print(value);
        printf(");");                
}
void emit_SCALAR(scalar_rule *scalar,rope *value){
        write_parser_bind(&scalar->parser,value);
}
void emit_OPTIONAL(optional_rule *optional,rope *value)
{
        printf("if(");
        rope_print(value);
        printf("){");
        emit_parserrule(&optional->inner,rope_const("[0]",value));
        printf("}");
}
void emit_CHOICE(choice_rule *choice,rope *value)
{
        printf("switch(");
        rope_print(value);
        printf(".N_TYPE){");
        FOREACH(option,choice->options){
                printf("case %.*s:\n", p_arg(option->tag));
                emit_parserrule(&option->inner,rope_narray(option->tag,rope_const("->",value)));
        }
        printf("default: return NULL;");
        printf("}");
} 
void emit_NX_LENGTH(nx_length_rule *length,rope *value){
        write_parser_bind(&length->lengthparser, rope_const(".count",value)); //Write out dummy value
        printf("FOREACH(iter,");
        rope_print(value);
        printf("){\n");
        emit_parserrule(&length->inner,rope_const("iter",NULL));
        printf("}\n");
}
void emit_parserrule(parserrule *rule,rope *value){
        switch(rule->N_type){
#define gen(x) case P_ ## x:                        \
                emit_## x (rule->P_##x,value);      \
                        break;

                            gen(STRUCT);
                            gen(ARRAY);
                            gen(REF);
                            gen(EMBED);
                            gen(SCALAR);
                            gen(OPTIONAL);
                            gen(CHOICE);
                            gen(NX_LENGTH);
                    default:
                            assert("boom");
  }
}
int main()
{
      
    uint8_t input[102400];
    size_t inputsize;
    size_t outputsize;
    char *out;
    const struct grammar *result;
    inputsize = fread(input, 1, sizeof(input), stdin);
    //fprintf(stderr, "inputsize=%zu\ninput=", inputsize);
    //fwrite(input, 1, inputsize, stderr);  
    // print_parser_invocation(parse_parser_invocation(input,inputsize),stdout,0);
    // exit(0);
     result =  parse_grammar(input,inputsize);
    if(result) {
            FOREACH(definition,result->rules){
                    printf("char * gen_%.*s(%.*s *val){\n",definition->name.count,definition->name.elem,definition->name.count,definition->name.elem);
                    emit_parserrule(&definition->rule,rope_const("val",NULL));
                    printf("}\n");                    
            }
            return 0;
    } else {
        return 1;
    }
}
