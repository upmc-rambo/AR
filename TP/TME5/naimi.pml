#define N      3    /* Number of processes */ 
#define L     10    /* Buffer size */
#define NIL (N+1)   /* for an undefined value */

LTL vivacity { [] (dem_SC -> ( <> in_SC ))};

mtype = {req, tk};

/* le processus i recoit ses messages dans canal[i] */ 
chan canal[N] = [L] of {mtype, byte};    /* pour un message de type tk, 
                                            on mettra la valeur associee a 0 */

byte SharedVar = 0;                      /* la variable partagee */ 

inline Initialisation ( ) {
   /* initialise les variables locales, sauf reqId, val et typ_mes */
  father = 0;
  next = NIL;
  requesting = false;
  token = (father == id);
  if 
	::	(father == id) -> father = NIL;
	:: else -> ;
  fi;
}

inline Request_CS ( ) {
  /* operations effectuees lors d'une demande de SC */
  requesting = true;
  if
    :: (father != NIL) -> canal[father]!req,id; father = NIL;
    :: else -> ;
  fi;

}

inline Release_CS ( ) {
  /* operations effectuees en sortie de SC */
  requesting = false;
  if
    :: (next != NIL) -> canal[next]!tk,0; token = false; next = NIL;
    :: else -> ;
  fi;
}

inline Receive_Request_CS ( ) {
  /* traitement de la reception d'une requete */
  if
    :: (father == NIL) ->
	if 
	  :: (requesting) -> next = reqId;
	  :: else -> token = false; canal[reqId]!tk,0;
	fi;
	:: else -> canal[father]!req,reqId;
  fi;
  father = reqId;

}

inline Receive_Token () {
  /* traitement de la reception du jeton */
	token = true;
}


proctype node(byte id; byte Initial_Token_Holder){
    
   bit requesting;    /* indique si le processus a demande la SC ou pas  */
   bit token;         /* indique si le processus possede le jeton ou non */

   byte father;       /* probable owner */
   byte next;         /* next node to whom send the token */
   byte val;          /* la valeur contenue dans le message */
   mtype typ_mes;     /* le type du message recu */
   byte reqId;        /* l'Id du demandeur, pour une requete */

   chan canal_rec = canal[id];     /* un alias pour mon canal de reception */
   xr canal_rec;                   /* je dois etre le seul a le lire */

   /* Chaque processus execute une boucle infinie */

   Initialisation ( );
	printf("Init\n");
   do
       :: ((token == true) && empty(canal_rec) && (requesting == true)) ->
	      /* on oblige le detenteur du jeton a consulter les messages recus */
	      in_SC :
				SharedVar++;
				printf("Section critique proc:%d\n", id);
				assert(SharedVar == 1);
				SharedVar--; // acces a la ressource critique (actions sur SharedVar), puis sortie de SC 
				Release_CS ( );

       :: canal_rec ? typ_mes(val) ->
	    /* traitement du message recu */
	    if
			:: (typ_mes == req) -> reqId = val; Receive_Request_CS();
			:: (typ_mes == tk) -> Receive_Token();
			:: else -> ;
        fi;

       :: (requesting == false) -> /* demander la SC */
	    dem_SC :
               Request_CS ( );
   od ;
}

/* Cree un ensemble de N processus */ 

init {
   byte proc; 
   atomic {
      proc=0;
      do
	:: (proc < N) ->
	     run node(proc, 0); 
            proc++
         :: proc == N -> break 
      od
   }
}
