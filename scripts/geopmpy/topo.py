import cffi

class Topo(object):
    def __init__(self):
        topo_header_txt = """
enum geopm_domain_e {
    GEOPM_DOMAIN_INVALID = -1,
    GEOPM_DOMAIN_BOARD = 0,
    GEOPM_DOMAIN_PACKAGE = 1,
    GEOPM_DOMAIN_CORE = 2,
    GEOPM_DOMAIN_CPU = 3,
    GEOPM_DOMAIN_BOARD_MEMORY = 4,
    GEOPM_DOMAIN_PACKAGE_MEMORY = 5,
    GEOPM_DOMAIN_BOARD_NIC = 6,
    GEOPM_DOMAIN_PACKAGE_NIC = 7,
    GEOPM_DOMAIN_BOARD_ACCELERATOR = 8,
    GEOPM_DOMAIN_PACKAGE_ACCELERATOR = 9,
    GEOPM_NUM_DOMAIN = 10,
};

int geopm_topo_num_domain(int domain_type);

int geopm_topo_domain_idx(int domain_type,
                          int cpu_idx);

int geopm_topo_num_domain_nested(int inner_domain,
                                 int outer_domain);

int geopm_topo_domain_nested(int inner_domain,
                             int outer_domain,
                             int outer_idx,
                             size_t num_domain_nested,
                             int *domain_nested);

int geopm_topo_domain_name(int domain_type,
                           size_t domain_name_max,
                           char *domain_name);

int geopm_topo_domain_type(const char *domain_name);

int geopm_topo_create_cache(void);
"""
        self._ffi = cffi.FFI()
        self._ffi.cdef(topo_header_txt)
        self._dl = self._ffi.dlopen('libgeopmpolicy.so')
        self.DOMAIN_INVALID = self._dl.GEOPM_DOMAIN_INVALID
        self.DOMAIN_BOARD = self._dl.GEOPM_DOMAIN_BOARD
        self.DOMAIN_PACKAGE = self._dl.GEOPM_DOMAIN_PACKAGE
        self.DOMAIN_CORE = self._dl.GEOPM_DOMAIN_CORE
        self.DOMAIN_CPU = self._dl.GEOPM_DOMAIN_CPU
        self.DOMAIN_BOARD_MEMORY = self._dl.GEOPM_DOMAIN_BOARD_MEMORY
        self.DOMAIN_PACKAGE_MEMORY = self._dl.GEOPM_DOMAIN_PACKAGE_MEMORY
        self.DOMAIN_BOARD_NIC = self._dl.GEOPM_DOMAIN_BOARD_NIC
        self.DOMAIN_PACKAGE_NIC = self._dl.GEOPM_DOMAIN_PACKAGE_NIC
        self.DOMAIN_BOARD_ACCELERATOR = self._dl.GEOPM_DOMAIN_BOARD_ACCELERATOR
        self.DOMAIN_PACKAGE_ACCELERATOR = self._dl.GEOPM_DOMAIN_PACKAGE_ACCELERATOR
        self.NUM_DOMAIN = self._dl.GEOPM_NUM_DOMAIN

    def num_domain(self, domain_type):
        if type(domain_type) == str:
            domain_type = self.domain_type(domain_type)
        result = self._dl.geopm_topo_num_domain(domain_type)
        if result < 0:
            raise ValueError("Domain value {} unknown".format(domain_type))
        return result

    def domain_idx(self, domain_type, cpu_idx):
        if type(domain_type) == str:
            domain_type = self.domain_type(domain_type)
        cpu_idx = int(cpu_idx)
        result = self._dl.geopm_topo_domain_idx(domain_type, cpu_idx)
        if result < 0:
            raise RuntimeError("geopm_topo_domain_idx({}, {}) failed".format(
                               domain_type, cpu_idx))
        return result

    def domain_nested(self, inner_domain, outer_domain, outer_idx):
        if type(inner_domain) == str:
            inner_domain = self.domain_type(inner_domain)
        if type(outer_domain) == str:
            outer_domain = self.domain_type(outer_domain)
        num_domain_nested = self._dl.geopm_topo_num_domain_nested(inner_domain, outer_domain)
        if (num_domain_nested < 0):
            raise RuntimeError("geopm_topo_num_domain_nested({}, {}) failed".format(
                               inner_domain, outer_domain))
        domain_nested_carr = self._ffi.new("int[]", num_domain_nested)
        self._dl.geopm_topo_domain_nested(inner_domain, outer_domain, outer_idx,
                                          num_domain_nested, domain_nested_carr)
        result = []
        for dom in domain_nested_carr:
            result.append(dom)
        return result

    def domain_name(self, domain_type):
        if type(domain_type) == str:
            domain_type = self.domain_type(domain_type)
        name_max = 1024
        result_cstr = self._ffi.new("char[]", name_max)
        err = self._dl.geopm_topo_domain_name(domain_type, name_max, result_cstr)
        if err < 0:
            raise RuntimeError("geopm_topo_domain_name({}, {}, result) failed".format(
                               domain_type, name_max))
        return self._ffi.string(result_cstr)

    def domain_type(self, domain_name):
        domain_name_cstr = self._ffi.new("char[]", str(domain_name))
        result = self._dl.geopm_topo_domain_type(domain_name_cstr)
        if result < 0:
            raise KeyError("Domain type {} unknown".format(domain_name))
        return result

    def create_cache(self):
        err = self._dl.geopm_topo_create_cache()
        if err < 0:
            raise RuntimeError("geopm_topo_create_cache() failed")

if __name__ == "__main__":
    import sys

    tt = Topo()
    sys.stdout.write("Number of cpus: {}\n".format(tt.num_domain("cpu")))
    sys.stdout.write("Number of packages: {}\n".format(tt.num_domain(tt.DOMAIN_PACKAGE)))
    sys.stdout.write("CPU 1 is on package {}\n".format(tt.domain_idx(tt.DOMAIN_PACKAGE, 1)))
    sys.stdout.write("domain_nested('cpu', 'package', 0) = {}\n".format(tt.domain_nested('cpu', 'package', 0)))
    sys.stdout.write("domain_name(3) = {}\n".format(tt.domain_name(3)))
    sys.stdout.write("domain_type('cpu') = {}\n".format(tt.domain_type('cpu')))
    try:
        sys.stdout.write("Number of cpus: {}\n".format(tt.num_domain("foobar")))
        raise Exception("Topo.num_domain('foobar') failed to raise exception")
    except KeyError:
        pass
    try:
        sys.stdout.write("Number of cpus: {}\n".format(tt.num_domain(100)))
        raise Exception("Topo.num_domain(100) failed to raise exception")
    except ValueError:
        pass
