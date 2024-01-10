FROM python:3.12

USER root
COPY problem.py .
RUN pip install pycryptodome
RUN apt-get update && apt-get install socat -y
ENV FLAG=SECCON{y0u_c4N_DiSCarD_a-HaNd_0f_cArdS_WithOut_RevEAliNg_When_Y0u_lOSe_at_poker}
CMD ["socat", "TCP-L:8080,fork,reuseaddr", "EXEC:'python3 problem.py'"]
