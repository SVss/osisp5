# osisp5

Exaclty N threads at a time without mutexes:

* create global array of flags
* monitor it in main thread by iterating through
<br/><br/>
* when thread #i starts - set [i] to "true"
* when thread #i exits - set [i] to "false"
<br/><br/>
* when creating a thread it's given an address to set flag
