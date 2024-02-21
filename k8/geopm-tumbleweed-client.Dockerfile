FROM opensuse/tumbleweed

RUN zypper install -y curl
RUN curl -fsSL https://download.opensuse.org/repositories/home:/cmcantal:/cloud/openSUSE_Tumbleweed/repodata/repomd.xml.key > /tmp/suse-key
RUN rpm --import /tmp/suse-key
RUN zypper addrepo https://download.opensuse.org/repositories/home:/cmcantal:/cloud/openSUSE_Tumbleweed/home:cmcantal:cloud.repo
ADD "https://www.random.org/cgi-bin/randbyte?nbytes=10format=h" skipcache
RUN zypper install -y geopm-service geopm-runtime
