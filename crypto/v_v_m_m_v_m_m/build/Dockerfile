FROM sagemath/sagemath:9.6

USER root
COPY problem.sage .
RUN apt-get update && apt-get install socat -y
RUN sage --pip install pycryptodome
ENV FLAG=SECCON{beE_bee=KeY_KEy-B3e_kEY-Key-LET_THE_BASS_KICK-vec-vec-mat-mat-vec-mat-mat...}
CMD ["socat", "TCP-L:8080,fork,reuseaddr", "EXEC:'sage problem.sage'"]