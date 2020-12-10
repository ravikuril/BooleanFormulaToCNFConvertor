# Boolean Formula To CNF Convertor

Follow below mentioned steps to convert boolean formules to cnf format.

s="p imp q imp r;";
Pass Boolean formula as a string mentioned above in CNF_convertor(Formula_string)


a) Boolean Formula String
s="p imp q imp r;";


CNF: (not(p) or (not(q) or r))

b) s="p imp q or r imp t;";

CNF: ((not(p) or (not(q) or t)) and (not(p) or (not(r) or t)))



c) s="p imp q and r imp not t;";


CNF: ((not(p) or (not(q) or t)) and (not(p) or (not(r) or t)))


d) s="p imp not q or r imp not t;";


CNF: ((not(p) or (q or not(t))) and (not(p) or (not(r) or not(t))))

e) s="p imp not q;";


CNF: (not(p) or not(q))


