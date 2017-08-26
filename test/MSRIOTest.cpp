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
#include "gtest/gtest.h"

#include "MSRIO.hpp"
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
        ftruncate(fd, M_MAX_OFFSET);
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
        "absolute",
        "abstract",
        "academic",
        "accepted",
        "accident",
        "accuracy",
        "accurate",
        "achieved",
        "acquired",
        "activity",
        "actually",
        "addition",
        "adequate",
        "adjacent",
        "adjusted",
        "advanced",
        "advisory",
        "advocate",
        "affected",
        "aircraft",
        "alliance",
        "although",
        "aluminum",
        "analysis",
        "announce",
        "anything",
        "anywhere",
        "apparent",
        "appendix",
        "approach",
        "approval",
        "argument",
        "artistic",
        "assembly",
        "assuming",
        "athletic",
        "attached",
        "attitude",
        "attorney",
        "audience",
        "autonomy",
        "aviation",
        "bachelor",
        "bacteria",
        "baseball",
        "bathroom",
        "becoming",
        "benjamin",
        "birthday",
        "boundary",
        "breaking",
        "breeding",
        "building",
        "bulletin",
        "business",
        "calendar",
        "campaign",
        "capacity",
        "casualty",
        "catching",
        "category",
        "Catholic",
        "cautious",
        "cellular",
        "ceremony",
        "chairman",
        "champion",
        "chemical",
        "children",
        "circular",
        "civilian",
        "clearing",
        "clinical",
        "clothing",
        "collapse",
        "colonial",
        "colorful",
        "commence",
        "commerce",
        "complain",
        "complete",
        "composed",
        "compound",
        "comprise",
        "computer",
        "conclude",
        "concrete",
        "conflict",
        "confused",
        "congress",
        "consider",
        "constant",
        "consumer",
        "continue",
        "contract",
        "contrary",
        "contrast",
        "convince",
        "corridor",
        "coverage",
        "covering",
        "creation",
        "creative",
        "criminal",
        "critical",
        "crossing",
        "cultural",
        "currency",
        "customer",
        "database",
        "daughter",
        "daylight",
        "deadline",
        "deciding",
        "decision",
        "decrease",
        "deferred",
        "definite",
        "delicate",
        "delivery",
        "describe",
        "designer",
        "detailed",
        "diabetes",
        "dialogue",
        "diameter",
        "directly",
        "director",
        "disabled",
        "disaster",
        "disclose",
        "discount",
        "discover",
        "disorder",
        "disposal",
        "distance",
        "distinct",
        "district",
        "dividend",
        "division",
        "doctrine",
        "document",
        "domestic",
        "dominant",
        "dominate",
        "doubtful",
        "dramatic",
        "dressing",
        "dropping",
        "duration",
        "dynamics",
        "earnings",
        "economic",
        "educated",
        "efficacy",
        "eighteen",
        "election",
        "electric",
        "eligible",
        "emerging",
        "emphasis",
        "employee",
        "endeavor",
        "engaging",
        "engineer",
        "enormous",
        "entirely",
        "entrance",
        "envelope",
        "equality",
        "equation",
        "estimate",
        "evaluate",
        "eventual",
        "everyday",
        "everyone",
        "evidence",
        "exchange",
        "exciting",
        "exercise",
        "explicit",
        "exposure",
        "extended",
        "external",
        "facility",
        "familiar",
        "featured",
        "feedback",
        "festival",
        "finished",
        "firewall",
        "flagship",
        "flexible",
        "floating",
        "football",
        "foothill",
        "forecast",
        "foremost",
        "formerly",
        "fourteen",
        "fraction",
        "franklin",
        "frequent",
        "friendly",
        "frontier",
        "function",
        "generate",
        "generous",
        "genomics",
        "goodwill",
        "governor",
        "graduate",
        "graphics",
        "grateful",
        "guardian",
        "guidance",
        "handling",
        "hardware",
        "heritage",
        "highland",
        "historic",
        "homeless",
        "homepage",
        "hospital",
        "humanity",
        "identify",
        "identity",
        "ideology",
        "imperial",
        "incident",
        "included",
        "increase",
        "indicate",
        "indirect",
        "industry",
        "informal",
        "informed",
        "inherent",
        "initiate",
        "innocent",
        "inspired",
        "instance",
        "integral",
        "intended",
        "interact",
        "interest",
        "interior",
        "internal",
        "interval",
        "intimate",
        "intranet",
        "invasion",
        "involved",
        "isolated",
        "judgment",
        "judicial",
        "junction",
        "keyboard",
        "landlord",
        "language",
        "laughter",
        "learning",
        "leverage",
        "lifetime",
        "lighting",
        "likewise",
        "limiting",
        "literary",
        "location",
        "magazine",
        "magnetic",
        "maintain",
        "majority",
        "marginal",
        "marriage",
        "material",
        "maturity",
        "maximize",
        "meantime",
        "measured",
        "medicine",
        "medieval",
        "memorial",
        "merchant",
        "midnight",
        "military",
        "minimize",
        "minister",
        "ministry",
        "minority",
        "mobility",
        "modeling",
        "moderate",
        "momentum",
        "monetary",
        "moreover",
        "mortgage",
        "mountain",
        "mounting",
        "movement",
        "multiple",
        "national",
        "negative",
        "nineteen",
        "northern",
        "notebook",
        "numerous",
        "observer",
        "occasion",
        "offering",
        "official",
        "offshore",
        "operator",
        "opponent",
        "opposite",
        "optimism",
        "optional",
        "ordinary",
        "organize",
        "original",
        "overcome",
        "overhead",
        "overseas",
        "overview",
        "painting",
        "parallel",
        "parental",
        "patented",
        "patience",
        "peaceful",
        "periodic",
        "personal",
        "persuade",
        "petition",
        "physical",
        "pipeline",
        "platform",
        "pleasant",
        "pleasure",
        "politics",
        "portable",
        "portrait",
        "position",
        "positive",
        "possible",
        "powerful",
        "practice",
        "precious",
        "pregnant",
        "presence",
        "preserve",
        "pressing",
        "pressure",
        "previous",
        "princess",
        "printing",
        "priority",
        "probable",
        "probably",
        "producer",
        "profound",
        "progress",
        "property",
        "proposal",
        "prospect",
        "protocol",
        "provided",
        "provider",
        "province",
        "publicly",
        "purchase",
        "pursuant",
        "quantity",
        "question",
        "rational",
        "reaction",
        "received",
        "receiver",
        "recovery",
        "regional",
        "register",
        "relation",
        "relative",
        "relevant",
        "reliable",
        "reliance",
        "religion",
        "remember",
        "renowned",
        "repeated",
        "reporter",
        "republic",
        "required",
        "research",
        "reserved",
        "resident",
        "resigned",
        "resource",
        "response",
        "restrict",
        "revision",
        "rigorous",
        "romantic",
        "sampling",
        "scenario",
        "schedule",
        "scrutiny",
        "seasonal",
        "secondly",
        "security",
        "sensible",
        "sentence",
        "separate",
        "sequence",
        "sergeant",
        "shipping",
        "shortage",
        "shoulder",
        "simplify",
        "situated",
        "slightly",
        "software",
        "solution",
        "somebody",
        "somewhat",
        "southern",
        "speaking",
        "specific",
        "spectrum",
        "sporting",
        "standard",
        "standing",
        "standout",
        "sterling",
        "straight",
        "strategy",
        "strength",
        "striking",
        "struggle",
        "stunning",
        "suburban",
        "suitable",
        "superior",
        "supposed",
        "surgical",
        "surprise",
        "survival",
        "sweeping",
        "swimming",
        "symbolic",
        "sympathy",
        "syndrome",
        "tactical",
        "tailored",
        "takeover",
        "tangible",
        "taxation",
        "taxpayer",
        "teaching",
        "tendency",
        "terminal",
        "terrible",
        "thinking",
        "thirteen",
        "thorough",
        "thousand",
        "together",
        "tomorrow",
        "touching",
        "tracking",
        "training",
        "transfer",
        "traveled",
        "treasury",
        "triangle",
        "tropical",
        "turnover",
        "ultimate",
        "umbrella",
        "universe",
        "unlawful",
        "unlikely",
        "valuable",
        "variable",
        "vertical",
        "victoria",
        "violence",
        "volatile",
        "warranty",
        "weakness",
        "weighted",
        "whatever",
        "whenever",
        "wherever",
        "wildlife",
        "wireless",
        "withdraw",
        "woodland",
        "workshop",
        "yourself",
        "aaaaaaaa",
        "bbbbbbbb",
        "cccccccc",
        "dddddddd",
        "eeeeeeee",
        "ffffffff",
        "gggggggg",
        "hhhhhhhh",
        "iiiiiiii",
        "jjjjjjjj",
        "kkkkkkkk",
        "llllllll",
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

    field = m_msrio->read_msr(0, 0);
    ASSERT_EQ(0, memcmp(&field, "absolute", 8));
    space_ptr = m_msrio->msr_space_ptr(0, 0);
    ASSERT_EQ(0, memcmp(&field, space_ptr, 8));

    field = m_msrio->read_msr(0, 1600);
    ASSERT_EQ(0, memcmp(&field, "fraction", 8));
    space_ptr = m_msrio->msr_space_ptr(0, 1600);
    ASSERT_EQ(0, memcmp(&field, space_ptr, 8));

    field = m_msrio->read_msr(1, 0);
    ASSERT_EQ(0, memcmp(&field, "absolute", 8));
    space_ptr = m_msrio->msr_space_ptr(1, 0);
    ASSERT_EQ(0, memcmp(&field, space_ptr, 8));

    field = m_msrio->read_msr(1, 1600);
    ASSERT_EQ(0, memcmp(&field, "fraction", 8));
    space_ptr = m_msrio->msr_space_ptr(1, 1600);
    ASSERT_EQ(0, memcmp(&field, space_ptr, 8));
}

TEST_F(MSRIOTest, read_unaligned)
{
    uint64_t field;
    char *space_ptr;

    field = m_msrio->read_msr(0, 4);
    ASSERT_EQ(0, memcmp(&field, "luteabst", 8));
    space_ptr = m_msrio->msr_space_ptr(0, 4);
    ASSERT_EQ(0, memcmp(&field, space_ptr, 8));

    field = m_msrio->read_msr(0, 1604);
    ASSERT_EQ(0, memcmp(&field, "tionfran", 8));
    space_ptr = m_msrio->msr_space_ptr(0, 1604);
    ASSERT_EQ(0, memcmp(&field, space_ptr, 8));

    field = m_msrio->read_msr(1, 4);
    ASSERT_EQ(0, memcmp(&field, "luteabst", 8));
    space_ptr = m_msrio->msr_space_ptr(1, 4);
    ASSERT_EQ(0, memcmp(&field, space_ptr, 8));

    field = m_msrio->read_msr(1, 1604);
    ASSERT_EQ(0, memcmp(&field, "tionfran", 8));
    space_ptr = m_msrio->msr_space_ptr(1, 1604);
    ASSERT_EQ(0, memcmp(&field, space_ptr, 8));
}

TEST_F(MSRIOTest, write)
{
    uint64_t field;
    char *space_ptr;

    memcpy(&field, "etul\0\0\0\0", 8);
    m_msrio->write_msr(0, 0, field, 0x00000000FFFFFFFF);
    space_ptr = m_msrio->msr_space_ptr(0, 0);
    ASSERT_EQ(0, memcmp(space_ptr, "etullute", 8));

    memcpy(&field, "\0\0\0\0osba", 8);
    m_msrio->write_msr(0, 0, field, 0xFFFFFFFF00000000);
    space_ptr = m_msrio->msr_space_ptr(0, 0);
    ASSERT_EQ(0, memcmp(space_ptr, "etulosba", 8));

    memcpy(&field, "noit\0\0\0\0", 8);
    m_msrio->write_msr(0, 1600, field, 0x00000000FFFFFFFF);
    space_ptr = m_msrio->msr_space_ptr(0, 1600);
    ASSERT_EQ(0, memcmp(space_ptr, "noittion", 8));

    memcpy(&field, "\0\0\0\0carf", 8);
    m_msrio->write_msr(0, 1600, field, 0xFFFFFFFF00000000);
    space_ptr = m_msrio->msr_space_ptr(0, 1600);
    ASSERT_EQ(0, memcmp(space_ptr, "noitcarf", 8));
}
