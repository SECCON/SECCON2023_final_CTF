# FROM sagemath/sagemath:10.1
FROM sagemath/sagemath:9.6

USER root

RUN apt update && apt install make libboost-all-dev git -y

ENV TERM=xterm
RUN sage --pip install pycryptodome pwntools

RUN git clone https://github.com/WardBeullens/BreakingRainbow.git
COPY . .
RUN patch -p1 < patch.patch

CMD ["sage", "solver.sage"]
