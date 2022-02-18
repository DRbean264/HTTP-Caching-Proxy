FROM ubuntu:20.04
RUN apt-get update
RUN apt-get install -y g++ make
RUN mkdir /var/log/erss
RUN touch /var/log/erss/proxy.log
RUN touch /var/log/erss/err.log
RUN mkdir /myproxy
WORKDIR /myproxy
ADD . /myproxy/
RUN ls /myproxy
RUN make
RUN chmod +x ./run_proxy.sh
