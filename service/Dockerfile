FROM opensuse/leap
RUN mkdir -m 700 -p /etc/geopm-service
RUN mkdir -m 700 -p /etc/geopm-service/0.DEFAULT_ACCESS
RUN echo "TIME" > /etc/geopm-service/0.DEFAULT_ACCESS/allowed_signals
RUN chmod 600 /etc/geopm-service/0.DEFAULT_ACCESS/allowed_signals
RUN zypper install -y curl
RUN curl -fsSL https://download.opensuse.org/repositories/home:/cmcantal/15.4/repodata/repomd.xml.key > /tmp/suse-key
RUN rpm --import /tmp/suse-key
RUN zypper addrepo https://download.opensuse.org/repositories/home:/cmcantal/15.4/home:cmcantal.repo
RUN zypper install -y geopm-service
RUN mkdir -p -m 755 /var; mkdir -p -m 755 /var/lib; mkdir -p -m 755 /var/lib/kubelet; mkdir -p -m 755 /var/lib/kubelet/seccomp; mkdir -p -m 755 /var/lib/kubelet/seccomp/profiles;
COPY geopm-*-seccomp.json /var/lib/kubelet/seccomp/profiles
RUN zypper install cargo
RUN zypper source-install -y geopm-service
WORKDIR /tmp
RUN tar xvf /usr/src/packages/SOURCES/geopm-service.tar.gz
RUN cargo install --path=geopm-service-*/geopmdrs --bin geopmd-proxy --root=/usr
