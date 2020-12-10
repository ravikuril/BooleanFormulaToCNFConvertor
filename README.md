# BooleanFormulaToCNFConvertor

Follow below mentioned steps to convert boolean formules to cnf format.

s="p imp q imp r;";
1) pass Boolean formula as a string mentioned above in CNF_convertor(Formula_string)


Boolean Formula String
s="p imp q imp r;";


CNF: (not(p) or (not(q) or r))
