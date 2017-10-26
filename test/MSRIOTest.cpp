/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <string>
#include "gtest/gtest.h"

#include "MSRIO.hpp"
#include "Exception.hpp"
#include "geopm_sched.h"

// Class derived from MSRIO used to test MSRIO w/o accessing the msr
// device files.
class TestMSRIO : public geopm::MSRIO
{
    public:
        TestMSRIO();
        virtual ~TestMSRIO();
        char *msr_space_ptr(int cpu_idx, off_t offset);
    protected:
        void msr_path(int cpu_idx,
                      bool is_fallback,
                      std::string &path);
        void msr_batch_path(std::string &path);
        const char **msr_words(void) const;

        const size_t M_MAX_OFFSET;
        const int m_num_cpu;
        std::vector<std::string> m_test_dev_path;
        std::vector<char *> m_msr_space;
};

TestMSRIO::TestMSRIO()
    : M_MAX_OFFSET(4096)
    , m_num_cpu(geopm_sched_num_cpu())
    , m_test_dev_path(m_num_cpu)
    , m_msr_space(m_num_cpu, NULL)
{
    for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
        std::ostringstream path;
        path << "test_msrio_dev_cpu_" << cpu_idx << "_msr_safe";
        m_test_dev_path[cpu_idx] = path.str();
    }
    auto msr_space_it = m_msr_space.begin();
    for (auto &path : m_test_dev_path) {
        int fd = open(path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        int err = ftruncate(fd, M_MAX_OFFSET);
        if (err) {
            throw geopm::Exception("TestMSRIO: ftruncate failed", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        char *msr_space_ptr = (char *)mmap(NULL, M_MAX_OFFSET, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        *msr_space_it = msr_space_ptr;
        size_t num_field = M_MAX_OFFSET / 8;
        for (size_t field_idx = 0; field_idx < num_field; ++field_idx) {
            std::copy(msr_words()[field_idx], msr_words()[field_idx] + 8, msr_space_ptr);
            msr_space_ptr += 8;
        }
        ++msr_space_it;
    }
}

TestMSRIO::~TestMSRIO()
{
    for (auto &msr_space_it : m_msr_space) {
        munmap(msr_space_it, M_MAX_OFFSET);
    }
    for (auto &path : m_test_dev_path) {
        unlink(path.c_str());
    }
}

void TestMSRIO::msr_path(int cpu_idx,
                         bool is_fallback,
                         std::string &path)
{
    path = m_test_dev_path[cpu_idx];
}

void TestMSRIO::msr_batch_path(std::string &path)
{
    path = "test_dev_msr_safe";
}

char* TestMSRIO::msr_space_ptr(int cpu_idx, off_t offset)
{
    return m_msr_space[cpu_idx] + offset;
}

const char **TestMSRIO::msr_words(void) const
{
    static const char *instance[] = {
        "absolute", // 0x0
        "abstract", // 0x8
        "academic", // 0x10
        "accepted", // 0x18
        "accident", // 0x20
        "accuracy", // 0x28
        "accurate", // 0x30
        "achieved", // 0x38
        "acquired", // 0x40
        "activity", // 0x48
        "actually", // 0x50
        "addition", // 0x58
        "adequate", // 0x60
        "adjacent", // 0x68
        "adjusted", // 0x70
        "advanced", // 0x78
        "advisory", // 0x80
        "advocate", // 0x88
        "affected", // 0x90
        "aircraft", // 0x98
        "alliance", // 0xa0
        "although", // 0xa8
        "aluminum", // 0xb0
        "analysis", // 0xb8
        "announce", // 0xc0
        "anything", // 0xc8
        "anywhere", // 0xd0
        "apparent", // 0xd8
        "appendix", // 0xe0
        "approach", // 0xe8
        "approval", // 0xf0
        "argument", // 0xf8
        "artistic", // 0x100
        "assembly", // 0x108
        "assuming", // 0x110
        "athletic", // 0x118
        "attached", // 0x120
        "attitude", // 0x128
        "attorney", // 0x130
        "audience", // 0x138
        "autonomy", // 0x140
        "aviation", // 0x148
        "bachelor", // 0x150
        "bacteria", // 0x158
        "baseball", // 0x160
        "bathroom", // 0x168
        "becoming", // 0x170
        "benjamin", // 0x178
        "birthday", // 0x180
        "boundary", // 0x188
        "breaking", // 0x190
        "breeding", // 0x198
        "building", // 0x1a0
        "bulletin", // 0x1a8
        "business", // 0x1b0
        "calendar", // 0x1b8
        "campaign", // 0x1c0
        "capacity", // 0x1c8
        "casualty", // 0x1d0
        "catching", // 0x1d8
        "category", // 0x1e0
        "Catholic", // 0x1e8
        "cautious", // 0x1f0
        "cellular", // 0x1f8
        "ceremony", // 0x200
        "chairman", // 0x208
        "champion", // 0x210
        "chemical", // 0x218
        "children", // 0x220
        "circular", // 0x228
        "civilian", // 0x230
        "clearing", // 0x238
        "clinical", // 0x240
        "clothing", // 0x248
        "collapse", // 0x250
        "colonial", // 0x258
        "colorful", // 0x260
        "commence", // 0x268
        "commerce", // 0x270
        "complain", // 0x278
        "complete", // 0x280
        "composed", // 0x288
        "compound", // 0x290
        "comprise", // 0x298
        "computer", // 0x2a0
        "conclude", // 0x2a8
        "concrete", // 0x2b0
        "conflict", // 0x2b8
        "confused", // 0x2c0
        "congress", // 0x2c8
        "consider", // 0x2d0
        "constant", // 0x2d8
        "consumer", // 0x2e0
        "continue", // 0x2e8
        "contract", // 0x2f0
        "contrary", // 0x2f8
        "contrast", // 0x300
        "convince", // 0x308
        "corridor", // 0x310
        "coverage", // 0x318
        "covering", // 0x320
        "creation", // 0x328
        "creative", // 0x330
        "criminal", // 0x338
        "critical", // 0x340
        "crossing", // 0x348
        "cultural", // 0x350
        "currency", // 0x358
        "customer", // 0x360
        "database", // 0x368
        "daughter", // 0x370
        "daylight", // 0x378
        "deadline", // 0x380
        "deciding", // 0x388
        "decision", // 0x390
        "decrease", // 0x398
        "deferred", // 0x3a0
        "definite", // 0x3a8
        "delicate", // 0x3b0
        "delivery", // 0x3b8
        "describe", // 0x3c0
        "designer", // 0x3c8
        "detailed", // 0x3d0
        "diabetes", // 0x3d8
        "dialogue", // 0x3e0
        "diameter", // 0x3e8
        "directly", // 0x3f0
        "director", // 0x3f8
        "disabled", // 0x400
        "disaster", // 0x408
        "disclose", // 0x410
        "discount", // 0x418
        "discover", // 0x420
        "disorder", // 0x428
        "disposal", // 0x430
        "distance", // 0x438
        "distinct", // 0x440
        "district", // 0x448
        "dividend", // 0x450
        "division", // 0x458
        "doctrine", // 0x460
        "document", // 0x468
        "domestic", // 0x470
        "dominant", // 0x478
        "dominate", // 0x480
        "doubtful", // 0x488
        "dramatic", // 0x490
        "dressing", // 0x498
        "dropping", // 0x4a0
        "duration", // 0x4a8
        "dynamics", // 0x4b0
        "earnings", // 0x4b8
        "economic", // 0x4c0
        "educated", // 0x4c8
        "efficacy", // 0x4d0
        "eighteen", // 0x4d8
        "election", // 0x4e0
        "electric", // 0x4e8
        "eligible", // 0x4f0
        "emerging", // 0x4f8
        "emphasis", // 0x500
        "employee", // 0x508
        "endeavor", // 0x510
        "engaging", // 0x518
        "engineer", // 0x520
        "enormous", // 0x528
        "entirely", // 0x530
        "entrance", // 0x538
        "envelope", // 0x540
        "equality", // 0x548
        "equation", // 0x550
        "estimate", // 0x558
        "evaluate", // 0x560
        "eventual", // 0x568
        "everyday", // 0x570
        "everyone", // 0x578
        "evidence", // 0x580
        "exchange", // 0x588
        "exciting", // 0x590
        "exercise", // 0x598
        "explicit", // 0x5a0
        "exposure", // 0x5a8
        "extended", // 0x5b0
        "external", // 0x5b8
        "facility", // 0x5c0
        "familiar", // 0x5c8
        "featured", // 0x5d0
        "feedback", // 0x5d8
        "festival", // 0x5e0
        "finished", // 0x5e8
        "firewall", // 0x5f0
        "flagship", // 0x5f8
        "flexible", // 0x600
        "floating", // 0x608
        "football", // 0x610
        "foothill", // 0x618
        "forecast", // 0x620
        "foremost", // 0x628
        "formerly", // 0x630
        "fourteen", // 0x638
        "fraction", // 0x640
        "franklin", // 0x648
        "frequent", // 0x650
        "friendly", // 0x658
        "frontier", // 0x660
        "function", // 0x668
        "generate", // 0x670
        "generous", // 0x678
        "genomics", // 0x680
        "goodwill", // 0x688
        "governor", // 0x690
        "graduate", // 0x698
        "graphics", // 0x6a0
        "grateful", // 0x6a8
        "guardian", // 0x6b0
        "guidance", // 0x6b8
        "handling", // 0x6c0
        "hardware", // 0x6c8
        "heritage", // 0x6d0
        "highland", // 0x6d8
        "historic", // 0x6e0
        "homeless", // 0x6e8
        "homepage", // 0x6f0
        "hospital", // 0x6f8
        "humanity", // 0x700
        "identify", // 0x708
        "identity", // 0x710
        "ideology", // 0x718
        "imperial", // 0x720
        "incident", // 0x728
        "included", // 0x730
        "increase", // 0x738
        "indicate", // 0x740
        "indirect", // 0x748
        "industry", // 0x750
        "informal", // 0x758
        "informed", // 0x760
        "inherent", // 0x768
        "initiate", // 0x770
        "innocent", // 0x778
        "inspired", // 0x780
        "instance", // 0x788
        "integral", // 0x790
        "intended", // 0x798
        "interact", // 0x7a0
        "interest", // 0x7a8
        "interior", // 0x7b0
        "internal", // 0x7b8
        "interval", // 0x7c0
        "intimate", // 0x7c8
        "intranet", // 0x7d0
        "invasion", // 0x7d8
        "involved", // 0x7e0
        "isolated", // 0x7e8
        "judgment", // 0x7f0
        "judicial", // 0x7f8
        "junction", // 0x800
        "keyboard", // 0x808
        "landlord", // 0x810
        "language", // 0x818
        "laughter", // 0x820
        "learning", // 0x828
        "leverage", // 0x830
        "lifetime", // 0x838
        "lighting", // 0x840
        "likewise", // 0x848
        "limiting", // 0x850
        "literary", // 0x858
        "location", // 0x860
        "magazine", // 0x868
        "magnetic", // 0x870
        "maintain", // 0x878
        "majority", // 0x880
        "marginal", // 0x888
        "marriage", // 0x890
        "material", // 0x898
        "maturity", // 0x8a0
        "maximize", // 0x8a8
        "meantime", // 0x8b0
        "measured", // 0x8b8
        "medicine", // 0x8c0
        "medieval", // 0x8c8
        "memorial", // 0x8d0
        "merchant", // 0x8d8
        "midnight", // 0x8e0
        "military", // 0x8e8
        "minimize", // 0x8f0
        "minister", // 0x8f8
        "ministry", // 0x900
        "minority", // 0x908
        "mobility", // 0x910
        "modeling", // 0x918
        "moderate", // 0x920
        "momentum", // 0x928
        "monetary", // 0x930
        "moreover", // 0x938
        "mortgage", // 0x940
        "mountain", // 0x948
        "mounting", // 0x950
        "movement", // 0x958
        "multiple", // 0x960
        "national", // 0x968
        "negative", // 0x970
        "nineteen", // 0x978
        "northern", // 0x980
        "notebook", // 0x988
        "numerous", // 0x990
        "observer", // 0x998
        "occasion", // 0x9a0
        "offering", // 0x9a8
        "official", // 0x9b0
        "offshore", // 0x9b8
        "operator", // 0x9c0
        "opponent", // 0x9c8
        "opposite", // 0x9d0
        "optimism", // 0x9d8
        "optional", // 0x9e0
        "ordinary", // 0x9e8
        "organize", // 0x9f0
        "original", // 0x9f8
        "overcome", // 0xa00
        "overhead", // 0xa08
        "overseas", // 0xa10
        "overview", // 0xa18
        "painting", // 0xa20
        "parallel", // 0xa28
        "parental", // 0xa30
        "patented", // 0xa38
        "patience", // 0xa40
        "peaceful", // 0xa48
        "periodic", // 0xa50
        "personal", // 0xa58
        "persuade", // 0xa60
        "petition", // 0xa68
        "physical", // 0xa70
        "pipeline", // 0xa78
        "platform", // 0xa80
        "pleasant", // 0xa88
        "pleasure", // 0xa90
        "politics", // 0xa98
        "portable", // 0xaa0
        "portrait", // 0xaa8
        "position", // 0xab0
        "positive", // 0xab8
        "possible", // 0xac0
        "powerful", // 0xac8
        "practice", // 0xad0
        "precious", // 0xad8
        "pregnant", // 0xae0
        "presence", // 0xae8
        "preserve", // 0xaf0
        "pressing", // 0xaf8
        "pressure", // 0xb00
        "previous", // 0xb08
        "princess", // 0xb10
        "printing", // 0xb18
        "priority", // 0xb20
        "probable", // 0xb28
        "probably", // 0xb30
        "producer", // 0xb38
        "profound", // 0xb40
        "progress", // 0xb48
        "property", // 0xb50
        "proposal", // 0xb58
        "prospect", // 0xb60
        "protocol", // 0xb68
        "provided", // 0xb70
        "provider", // 0xb78
        "province", // 0xb80
        "publicly", // 0xb88
        "purchase", // 0xb90
        "pursuant", // 0xb98
        "quantity", // 0xba0
        "question", // 0xba8
        "rational", // 0xbb0
        "reaction", // 0xbb8
        "received", // 0xbc0
        "receiver", // 0xbc8
        "recovery", // 0xbd0
        "regional", // 0xbd8
        "register", // 0xbe0
        "relation", // 0xbe8
        "relative", // 0xbf0
        "relevant", // 0xbf8
        "reliable", // 0xc00
        "reliance", // 0xc08
        "religion", // 0xc10
        "remember", // 0xc18
        "renowned", // 0xc20
        "repeated", // 0xc28
        "reporter", // 0xc30
        "republic", // 0xc38
        "required", // 0xc40
        "research", // 0xc48
        "reserved", // 0xc50
        "resident", // 0xc58
        "resigned", // 0xc60
        "resource", // 0xc68
        "response", // 0xc70
        "restrict", // 0xc78
        "revision", // 0xc80
        "rigorous", // 0xc88
        "romantic", // 0xc90
        "sampling", // 0xc98
        "scenario", // 0xca0
        "schedule", // 0xca8
        "scrutiny", // 0xcb0
        "seasonal", // 0xcb8
        "secondly", // 0xcc0
        "security", // 0xcc8
        "sensible", // 0xcd0
        "sentence", // 0xcd8
        "separate", // 0xce0
        "sequence", // 0xce8
        "sergeant", // 0xcf0
        "shipping", // 0xcf8
        "shortage", // 0xd00
        "shoulder", // 0xd08
        "simplify", // 0xd10
        "situated", // 0xd18
        "slightly", // 0xd20
        "software", // 0xd28
        "solution", // 0xd30
        "somebody", // 0xd38
        "somewhat", // 0xd40
        "southern", // 0xd48
        "speaking", // 0xd50
        "specific", // 0xd58
        "spectrum", // 0xd60
        "sporting", // 0xd68
        "standard", // 0xd70
        "standing", // 0xd78
        "standout", // 0xd80
        "sterling", // 0xd88
        "straight", // 0xd90
        "strategy", // 0xd98
        "strength", // 0xda0
        "striking", // 0xda8
        "struggle", // 0xdb0
        "stunning", // 0xdb8
        "suburban", // 0xdc0
        "suitable", // 0xdc8
        "superior", // 0xdd0
        "supposed", // 0xdd8
        "surgical", // 0xde0
        "surprise", // 0xde8
        "survival", // 0xdf0
        "sweeping", // 0xdf8
        "swimming", // 0xe00
        "symbolic", // 0xe08
        "sympathy", // 0xe10
        "syndrome", // 0xe18
        "tactical", // 0xe20
        "tailored", // 0xe28
        "takeover", // 0xe30
        "tangible", // 0xe38
        "taxation", // 0xe40
        "taxpayer", // 0xe48
        "teaching", // 0xe50
        "tendency", // 0xe58
        "terminal", // 0xe60
        "terrible", // 0xe68
        "thinking", // 0xe70
        "thirteen", // 0xe78
        "thorough", // 0xe80
        "thousand", // 0xe88
        "together", // 0xe90
        "tomorrow", // 0xe98
        "touching", // 0xea0
        "tracking", // 0xea8
        "training", // 0xeb0
        "transfer", // 0xeb8
        "traveled", // 0xec0
        "treasury", // 0xec8
        "triangle", // 0xed0
        "tropical", // 0xed8
        "turnover", // 0xee0
        "ultimate", // 0xee8
        "umbrella", // 0xef0
        "universe", // 0xef8
        "unlawful", // 0xf00
        "unlikely", // 0xf08
        "valuable", // 0xf10
        "variable", // 0xf18
        "vertical", // 0xf20
        "victoria", // 0xf28
        "violence", // 0xf30
        "volatile", // 0xf38
        "warranty", // 0xf40
        "weakness", // 0xf48
        "weighted", // 0xf50
        "whatever", // 0xf58
        "whenever", // 0xf60
        "wherever", // 0xf68
        "wildlife", // 0xf70
        "wireless", // 0xf78
        "withdraw", // 0xf80
        "woodland", // 0xf88
        "workshop", // 0xf90
        "yourself", // 0xf98
        "aaaaaaaa", // 0xfa0
        "bbbbbbbb", // 0xfa8
        "cccccccc", // 0xfb0
        "dddddddd", // 0xfb8
        "eeeeeeee", // 0xfc0
        "ffffffff", // 0xfc8
        "gggggggg", // 0xfd0
        "hhhhhhhh", // 0xfd8
        "iiiiiiii", // 0xfe0
        "jjjjjjjj", // 0xfe8
        "kkkkkkkk", // 0xff0
        "llllllll", // 0xff8

    };
    return instance;
}


class MSRIOTest : public :: testing :: Test
{
    protected:
        void SetUp(void);
        void TearDown(void);
        TestMSRIO *m_msrio;
};

void MSRIOTest::SetUp(void)
{
    m_msrio = new TestMSRIO();
}

void MSRIOTest::TearDown(void)
{
    delete m_msrio;
}

TEST_F(MSRIOTest, read_aligned)
{
    uint64_t field;
    char *space_ptr;

    for (int cpu_idx = 0; cpu_idx < geopm_sched_num_cpu(); ++cpu_idx) {
        field = m_msrio->read_msr(cpu_idx, 0);
        ASSERT_EQ(0, memcmp(&field, "absolute", 8));
        space_ptr = m_msrio->msr_space_ptr(cpu_idx, 0);
        ASSERT_EQ(0, memcmp(&field, space_ptr, 8));

        field = m_msrio->read_msr(cpu_idx, 1600);
        ASSERT_EQ(0, memcmp(&field, "fraction", 8));
        space_ptr = m_msrio->msr_space_ptr(cpu_idx, 1600);
        ASSERT_EQ(0, memcmp(&field, space_ptr, 8));
    }
}

TEST_F(MSRIOTest, read_unaligned)
{
    uint64_t field;
    char *space_ptr;

    for (int cpu_idx = 0; cpu_idx < geopm_sched_num_cpu(); ++cpu_idx) {
        field = m_msrio->read_msr(cpu_idx, 4);
        ASSERT_EQ(0, memcmp(&field, "luteabst", 8));
        space_ptr = m_msrio->msr_space_ptr(cpu_idx, 4);
        ASSERT_EQ(0, memcmp(&field, space_ptr, 8));

        field = m_msrio->read_msr(cpu_idx, 1604);
        ASSERT_EQ(0, memcmp(&field, "tionfran", 8));
        space_ptr = m_msrio->msr_space_ptr(cpu_idx, 1604);
        ASSERT_EQ(0, memcmp(&field, space_ptr, 8));
    }
}

TEST_F(MSRIOTest, write)
{
    uint64_t field;
    char *space_ptr;

    for (int cpu_idx = 0; cpu_idx < geopm_sched_num_cpu(); ++cpu_idx) {
        memcpy(&field, "etul\0\0\0\0", 8);
        m_msrio->write_msr(cpu_idx, 0, field, 0x00000000FFFFFFFF);
        space_ptr = m_msrio->msr_space_ptr(cpu_idx, 0);
        ASSERT_EQ(0, memcmp(space_ptr, "etullute", 8));

        memcpy(&field, "\0\0\0\0osba", 8);
        m_msrio->write_msr(cpu_idx, 0, field, 0xFFFFFFFF00000000);
        space_ptr = m_msrio->msr_space_ptr(cpu_idx, 0);
        ASSERT_EQ(0, memcmp(space_ptr, "etulosba", 8));

        memcpy(&field, "noit\0\0\0\0", 8);
        m_msrio->write_msr(cpu_idx, 1600, field, 0x00000000FFFFFFFF);
        space_ptr = m_msrio->msr_space_ptr(cpu_idx, 1600);
        ASSERT_EQ(0, memcmp(space_ptr, "noittion", 8));

        memcpy(&field, "\0\0\0\0carf", 8);
        m_msrio->write_msr(cpu_idx, 1600, field, 0xFFFFFFFF00000000);
        space_ptr = m_msrio->msr_space_ptr(cpu_idx, 1600);
        ASSERT_EQ(0, memcmp(space_ptr, "noitcarf", 8));
    }
}

TEST_F(MSRIOTest, read_batch)
{
    std::vector<int> cpu_idx;
    for (int i = 0; i < geopm_sched_num_cpu(); ++i) {
        cpu_idx.push_back(i);
    }
    std::vector<std::string> words {"software", "engineer", "document", "everyday",
                                    "modeling", "standout", "patience", "goodwill"};
    std::vector<uint64_t> offsets {0xd28, 0x520, 0x468, 0x570, 0x918, 0xd80, 0xa40, 0x688};

    std::vector<int> read_cpu_idx;
    std::vector<uint64_t> read_offset;
    std::vector<uint64_t> expected;
    for (auto &ci : cpu_idx) {
        auto wi = words.begin();
        for (auto oi : offsets) {
            read_cpu_idx.push_back(ci);
            read_offset.push_back(oi);
            uint64_t result;
            memcpy(&result, (*wi).data(), 8);
            expected.push_back(result);
            ++wi;
        }
    }
    m_msrio->config_batch(read_cpu_idx, read_offset, {}, {}, {});
    std::vector<uint64_t> actual;
    m_msrio->read_batch(actual);
    EXPECT_EQ(expected, actual);
}

TEST_F(MSRIOTest, write_batch)
{
    std::vector<int> cpu_idx;
    for (int i = 0; i < geopm_sched_num_cpu(); ++i) {
        cpu_idx.push_back(i);
    }
    std::vector<std::string> begin_words {"software", "engineer", "document", "everyday",
                                          "modeling", "standout", "patience", "goodwill"};
    std::vector<std::string> end_words   {"HARDware", "BEgineRX", "Mocument", "everyWay",
                                          "moBIling", "XHandout", "patENTED", "goLdMill"};

    std::vector<uint64_t> offsets {0xd28, 0x520, 0x468, 0x570, 0x918, 0xd80, 0xa40, 0x688};
    std::vector<uint64_t> masks   {0x00000000FFFFFFFF,
                                   0xFFFF00000000FFFF,
                                   0x00000000000000FF,
                                   0x0000FF0000000000,
                                   0x00000000FFFF0000,
                                   0x000000000000FFFF,
                                   0xFFFFFFFFFF000000,
                                   0x000000FF00FF0000};
    std::vector<const char *> write_words  {"HARD\0\0\0\0", "BE\0\0\0\0RX", "M\0\0\0\0\0\0\0", "\0\0\0\0\0W\0\0",
                                            "\0\0BI\0\0\0\0", "XH\0\0\0\0\0\0", "\0\0\0ENTED", "\0\0L\0M\0\0\0"};

    std::vector<int> write_cpu_idx;
    std::vector<uint64_t> write_offset;
    std::vector<uint64_t> write_mask;
    std::vector<uint64_t> write_value;

    for (auto &ci : cpu_idx) {
        auto wi = write_words.begin();
        auto mi = masks.begin();
        for (auto oi : offsets) {
            write_cpu_idx.push_back(ci);
            write_offset.push_back(oi);
            write_mask.push_back(*mi);
            uint64_t result;
            memcpy(&result, (*wi), 8);
            write_value.push_back(result);
            ++wi;
            ++mi;
        }
    }
    m_msrio->config_batch({}, {}, write_cpu_idx, write_offset, write_mask);
    std::vector<uint64_t> result;
    m_msrio->write_batch(write_value);
    for (auto &ci : cpu_idx) {
        auto wi = end_words.begin();
        for (auto oi : offsets) {
            EXPECT_EQ(0, memcmp(m_msrio->msr_space_ptr(ci, oi), (*wi).c_str(), 8));
            ++wi;
        }
    }
}
