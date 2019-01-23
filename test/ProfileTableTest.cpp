/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include <stdlib.h>
#include "gtest/gtest.h"
#include "geopm_internal.h"
#include "Exception.hpp"
#include "ProfileTable.hpp"

class ProfileTableTest: public :: testing :: Test
{
    public:
        ProfileTableTest();
        virtual ~ProfileTableTest();
        void overfill_small(void);
    protected:
        size_t m_size;
        size_t m_small_size;
        char m_ptr[5192];
        char m_small_ptr[256];
        geopm::ProfileTable *m_table;
        geopm::ProfileTable *m_table_small;
};

ProfileTableTest::ProfileTableTest()
    : m_size(sizeof(m_ptr))
    , m_small_size(sizeof(m_small_ptr))
{
    m_table = new geopm::ProfileTable(m_size, (void *)m_ptr);
    m_table_small = new geopm::ProfileTable(m_small_size, (void *)m_small_ptr);
}

ProfileTableTest::~ProfileTableTest()
{
    delete m_table_small;
    delete m_table;
}

void ProfileTableTest::overfill_small(void)
{
    struct geopm_prof_message_s message;
    for (size_t i = 1; i <= m_table_small->capacity(); ++i) {
        message.progress = (double)i;
        message.region_id = i;
        m_table_small->insert(message);
    }
}

TEST_F(ProfileTableTest, overfill)
{
    overfill_small();
    struct geopm_prof_message_s message;
    message.progress = 0.5;
    message.region_id = 1234;
    EXPECT_THROW(m_table_small->insert(message), geopm::Exception);
}

TEST_F(ProfileTableTest, hello)
{
    struct geopm_prof_message_s insert_message;
    insert_message.progress = 1.234;
    insert_message.region_id = 1234;
    m_table->insert(insert_message);
    insert_message.progress = 5.678;
    insert_message.region_id = 5678;
    m_table->insert(insert_message);
    insert_message.progress = 9.876;
    insert_message.region_id = 5678;
    m_table->insert(insert_message);
    EXPECT_THROW(geopm::ProfileTable(0,NULL), geopm::Exception);
    uint64_t tmp[128];
    EXPECT_THROW(geopm::ProfileTable(1,tmp), geopm::Exception);
    uint64_t key0 = m_table->key("hello");
    uint64_t key1 = m_table->key("hello1");
    uint64_t key2 = m_table->key("hello");
    EXPECT_NE(key0, key1);
    EXPECT_EQ(key0, key2);
    insert_message.progress = 1234.5;
    insert_message.region_id = key0;
    m_table->insert(insert_message);
    std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > contents(3);
    size_t length;
    m_table->dump(contents.begin(), length);
    EXPECT_EQ(3ULL, length);
    for (int i = 0; i < 3; ++i) {
        if (contents[i].first == 1234) {
            EXPECT_EQ(1.234, contents[i].second.progress);
        }
        else if (contents[i].first == 5678) {
            EXPECT_EQ(9.876, contents[i].second.progress);
        }
        else if (contents[i].first == key0) {
            EXPECT_EQ(1234.5, contents[i].second.progress);
        }
        else {
            EXPECT_TRUE(false);
        }
    }
}

TEST_F(ProfileTableTest, name_set_fill_short)
{
    std::set<std::string> input_set = {"hello", "goodbye"};
    std::set<std::string> output_set;
    for (auto it = input_set.begin(); it != input_set.end(); ++it) {
        m_table->key(*it);
    }
    bool is_in_done = m_table->name_fill(0);
    bool is_out_done = m_table->name_set(0, output_set);
    ASSERT_EQ(input_set, output_set);
    ASSERT_EQ(is_in_done, is_out_done);
}
TEST_F(ProfileTableTest, name_set_fill_long)
{
    std::set<std::string> input_set = {
        "Global", "Extensible", "Open", "Power", "Manager", "GEOPM", "is", "an", "extensible", "power",
        "management", "framework", "targeting", "high", "performance", "computing", "The", "library", "can", "be",
        "extended", "to", "support", "new", "control", "algorithms", "and", "new", "hardware", "power", "management",
        "features", "The", "GEOPM", "package", "provides", "built", "in", "features", "ranging", "from", "static",
        "management", "of", "power", "policy", "for", "each", "individual", "compute", "node", "to", "dynamic",
        "coordination", "of", "power", "policy", "and", "performance", "across", "all", "of", "the", "compute", "nodes",
        "hosting", "one", "MPI", "job", "on", "a", "portion", "of", "a", "distributed", "computing", "system", "The",
        "dynamic", "coordination", "is", "implemented", "as", "a", "hierarchical", "control", "system", "for",
        "scalable", "communication", "and", "decentralized", "control", "The", "hierarchical", "control",
        "system", "can", "optimize", "for", "various", "objective", "functions", "including", "maximizing",
        "global", "application", "performance", "within", "a", "power", "bound", "The", "root", "of", "the", "control",
        "hierarchy", "tree", "can", "communicate", "through", "shared", "memory", "with", "the", "system", "resource",
        "management", "daemon", "to", "extend", "the", "hierarchy", "above", "the", "individual", "MPI", "job", "level",
        "and", "enable", "management", "of", "system", "power", "resources", "for", "multiple", "MPI", "jobs", "and",
        "multiple", "users", "by", "the", "system", "resource", "manager", "The", "geopm", "package", "provides", "the",
        "libgeopm", "library", "the", "libgeopmpolicy", "library", "the", "geopmctl", "application", "and", "the",
        "geopmpolicy", "application", "The", "libgeopm", "library", "can", "be", "called", "within", "MPI",
        "applications", "to", "enable", "application", "feedback", "for", "informing", "the", "control",
        "decisions", "If", "modification", "of", "the", "target", "application", "is", "not", "desired", "then", "the",
        "geopmctl", "application", "can", "be", "run", "concurrently", "with", "the", "target", "application", "In",
        "this", "case", "target", "application", "feedback", "is", "inferred", "by", "querying", "the", "hardware",
        "through", "Model", "Specific", "Registers", "MSRs", "With", "either", "method", "libgeopm", "or",
        "geopmctl", "the", "control", "hierarchy", "tree", "writes", "processor", "power", "policy", "through",
        "MSRs", "to", "enact", "policy", "decisions", "The", "libgeopmpolicy", "library", "is", "used", "by", "a",
        "resource", "manager", "to", "set", "energy", "policy", "control", "parameters", "for", "MPI", "jobs", "Some",
        "features", "of", "libgeopmpolicy", "are", "availble", "through", "the", "geopmpolicy", "application",
        "including", "support", "for", "static", "control",
        "When", "in", "the", "Course", "of", "human", "events,", "it", "becomes", "necessary", "for", "one",
        "people", "to", "dissolve", "the", "political", "bands", "which", "have", "connected", "them", "with",
        "another,", "and", "to", "assume", "among", "the", "powers", "of", "the", "earth,", "the", "separate", "and",
        "equal", "station", "to", "which", "the", "Laws", "of", "Nature", "and", "of", "Nature's", "God", "entitle",
        "them,", "a", "decent", "respect", "to", "the", "opinions", "of", "mankind", "requires", "that", "they",
        "should", "declare", "the", "causes", "which", "impel", "them", "to", "the", "separation.",
        "We", "hold", "these", "truths", "to", "be", "self-evident,", "that", "all", "men", "are", "created",
        "equal,", "that", "they", "are", "endowed", "by", "their", "Creator", "with", "certain", "unalienable",
        "Rights,", "that", "among", "these", "are", "Life,", "Liberty", "and", "the", "pursuit", "of",
        "Happiness.--That", "to", "secure", "these", "rights,", "Governments", "are", "instituted",
        "among", "Men,", "deriving", "their", "just", "powers", "from", "the", "consent", "of", "the",
        "governed,", "--That", "whenever", "any", "Form", "of", "Government", "becomes", "destructive",
        "of", "these", "ends,", "it", "is", "the", "Right", "of", "the", "People", "to", "alter", "or", "to", "abolish",
        "it,", "and", "to", "institute", "new", "Government,", "laying", "its", "foundation", "on", "such",
        "principles", "and", "organizing", "its", "powers", "in", "such", "form,", "as", "to", "them", "shall",
        "seem", "most", "likely", "to", "effect", "their", "Safety", "and", "Happiness.", "Prudence,",
        "indeed,", "will", "dictate", "that", "Governments", "long", "established", "should", "not", "be",
        "changed", "for", "light", "and", "transient", "causes;", "and", "accordingly", "all", "experience",
        "hath", "shewn,", "that", "mankind", "are", "more", "disposed", "to", "suffer,", "while", "evils", "are",
        "sufferable,", "than", "to", "right", "themselves", "by", "abolishing", "the", "forms", "to", "which",
        "they", "are", "accustomed.", "But", "when", "a", "long", "train", "of", "abuses", "and", "usurpations,",
        "pursuing", "invariably", "the", "same", "Object", "evinces", "a", "design", "to", "reduce", "them",
        "under", "absolute", "Despotism,", "it", "is", "their", "right,", "it", "is", "their", "duty,", "to",
        "throw", "off", "such", "Government,", "and", "to", "provide", "new", "Guards", "for", "their", "future",
        "security.--Such", "has", "been", "the", "patient", "sufferance", "of", "these", "Colonies;", "and",
        "such", "is", "now", "the", "necessity", "which", "constrains", "them", "to", "alter", "their", "former",
        "Systems", "of", "Government.", "The", "history", "of", "the", "present", "King", "of", "Great",
        "Britain", "is", "a", "history", "of", "repeated", "injuries", "and", "usurpations,", "all", "having",
        "in", "direct", "object", "the", "establishment", "of", "an", "absolute", "Tyranny", "over", "these",
        "States.", "To", "prove", "this,", "let", "Facts", "be", "submitted", "to", "a", "candid", "world.",
        "He", "has", "refused", "his", "Assent", "to", "Laws,", "the", "most", "wholesome", "and", "necessary",
        "for", "the", "public", "good.", "He", "has", "forbidden", "his", "Governors", "to", "pass", "Laws", "of",
        "immediate", "and", "pressing", "importance,", "unless", "suspended", "in", "their", "operation", "till",
        "his", "Assent", "should", "be", "obtained;", "and", "when", "so", "suspended,", "he", "has", "utterly",
        "neglected", "to", "attend", "to", "them.He", "has", "refused", "to", "pass", "other", "Laws", "for", "the",
        "accommodation", "of", "large", "districts", "of", "people,", "unless", "those", "people", "would", "relinquish",
        "the", "right", "of", "Representation", "in", "the", "Legislature,", "a", "right", "inestimable", "to", "them",
        "and", "formidable", "to", "tyrants", "only.", "He", "has", "called", "together", "legislative", "bodies", "at",
        "places", "unusual,", "uncomfortable,", "and", "distant", "from", "the", "depository", "of", "their", "public",
        "Records,", "for", "the", "sole", "purpose", "of", "fatiguing", "them", "into", "compliance", "with", "his",
        "measures.", "He", "has", "dissolved", "Representative", "Houses", "repeatedly,", "for", "opposing", "with",
        "manly", "firmness", "his", "invasions", "on", "the", "rights", "of", "the", "people.He", "has", "refused", "for",
        "a", "long", "time,", "after", "such", "dissolutions,", "to", "cause", "others", "to", "be", "elected;",
        "whereby", "the", "Legislative", "powers,", "incapable", "of", "Annihilation,", "have", "returned", "to", "the",
        "People", "at", "large", "for", "their", "exercise;", "the", "State", "remaining", "in", "the", "mean", "time",
        "exposed", "to", "all", "the", "dangers", "of", "invasion", "from", "without,", "and", "convulsions", "within.He",
        "has", "endeavoured", "to", "prevent", "the", "population", "of", "these", "States;", "for", "that", "purpose",
        "obstructing", "the", "Laws", "for", "Naturalization", "of", "Foreigners;", "refusing", "to", "pass", "others",
        "to", "encourage", "their", "migrations", "hither,", "and", "raising", "the", "conditions", "of", "new",
        "Appropriations", "of", "Lands.He", "has", "obstructed", "the", "Administration", "of", "Justice,", "by", "refusing",
        "his", "Assent", "to", "Laws", "for", "establishing", "Judiciary", "powers.He", "has", "made", "Judges", "dependent",
        "on", "his", "Will", "alone,", "for", "the", "tenure", "of", "their", "offices,", "and", "the", "amount", "and",
        "payment", "of", "their", "salaries.He", "has", "erected", "a", "multitude", "of", "New", "Offices,", "and", "sent",
        "hither", "swarms", "of", "Officers", "to", "harrass", "our", "people,", "and", "eat", "out", "their", "substance.He",
        "has", "kept", "among", "us,", "in", "times", "of", "peace,", "Standing", "Armies", "without", "the", "Consent", "of",
        "our", "legislatures.He", "has", "affected", "to", "render", "the", "Military", "independent", "of", "and", "superior",
        "to", "the", "Civil", "power.He", "has", "combined", "with", "others", "to", "subject", "us", "to", "a", "jurisdiction",
        "foreign", "to", "our", "constitution,", "and", "unacknowledged", "by", "our", "laws;", "giving", "his", "Assent", "to",
        "their", "Acts", "of", "pretended", "Legislation:For", "Quartering", "large", "bodies", "of", "armed", "troops",
        "among", "us:For", "protecting", "them,", "by", "a", "mock", "Trial,", "from", "punishment", "for", "any", "Murders",
        "which", "they", "should", "commit", "on", "the", "Inhabitants", "of", "these", "States:For", "cutting", "off", "our",
        "Trade", "with", "all", "parts", "of", "the", "world:For", "imposing", "Taxes", "on", "us", "without", "our",
        "Consent:", "For", "depriving", "us", "in", "many", "cases,", "of", "the", "benefits", "of", "Trial", "by", "Jury:For",
        "transporting", "us", "beyond", "Seas", "to", "be", "tried", "for", "pretended", "offencesFor", "abolishing", "the",
        "free", "System", "of", "English", "Laws", "in", "a", "neighbouring", "Province,", "establishing", "therein", "an",
        "Arbitrary", "government,", "and", "enlarging", "its", "Boundaries", "so", "as", "to", "render", "it", "at", "once",
        "an", "example", "and", "fit", "instrument", "for", "introducing", "the", "same", "absolute", "rule", "into", "these",
        "Colonies:For", "taking", "away", "our", "Charters,", "abolishing", "our", "most", "valuable", "Laws,", "and", "altering",
        "fundamentally", "the", "Forms", "of", "our", "Governments:For", "suspending", "our", "own", "Legislatures,", "and",
        "declaring", "themselves", "invested", "with", "power", "to", "legislate", "for", "us", "in", "all", "cases",
        "whatsoever.He", "has", "abdicated", "Government", "here,", "by", "declaring", "us", "out", "of", "his", "Protection",
        "and", "waging", "War", "against", "us.He", "has", "plundered", "our", "seas,", "ravaged", "our", "Coasts,", "burnt",
        "our", "towns,", "and", "destroyed", "the", "lives", "of", "our", "people.", "He", "is", "at", "this", "time", "transporting",
        "large", "Armies", "of", "foreign", "Mercenaries", "to", "compleat", "the", "works", "of", "death,", "desolation", "and",
        "tyranny,", "already", "begun", "with", "circumstances", "of", "Cruelty", "&", "perfidy", "scarcely", "paralleled", "in",
        "the", "most", "barbarous", "ages,", "and", "totally", "unworthy", "the", "Head", "of", "a", "civilized", "nation.He",
        "has", "constrained", "our", "fellow", "Citizens", "taken", "Captive", "on", "the", "high", "Seas", "to", "bear", "Arms",
        "against", "their", "Country,", "to", "become", "the", "executioners", "of", "their", "friends", "and", "Brethren,", "or",
        "to", "fall", "themselves", "by", "their", "Hands.", "He", "has", "excited", "domestic", "insurrections", "amongst", "us,",
        "and", "has", "endeavoured", "to", "bring", "on", "the", "inhabitants", "of", "our", "frontiers,", "the", "merciless", "Indian",
        "Savages,", "whose", "known", "rule", "of", "warfare,", "is", "an", "undistinguished", "destruction", "of", "all", "ages,",
        "sexes", "and", "conditions.In", "every", "stage", "of", "these", "Oppressions", "We", "have", "Petitioned", "for", "Redress",
        "in", "the", "most", "humble", "terms:", "Our", "repeated", "Petitions", "have", "been", "answered", "only", "by", "repeated",
        "injury.", "A", "Prince", "whose", "character", "is", "thus", "marked", "by", "every", "act", "which", "may", "define", "a",
        "Tyrant,", "is", "unfit", "to", "be", "the", "ruler", "of", "a", "free", "people.Nor", "have", "We", "been", "wanting",
        "in", "attentions", "to", "our", "Brittish", "brethren.", "We", "have", "warned", "them", "from", "time", "to", "time", "of",
        "attempts", "by", "their", "legislature", "to", "extend", "an", "unwarrantable", "jurisdiction", "over", "us.", "We", "have",
        "reminded", "them", "of", "the", "circumstances", "of", "our", "emigration", "and", "settlement", "here.", "We", "have",
        "appealed", "to", "their", "native", "justice", "and", "magnanimity,", "and", "we", "have", "conjured", "them", "by", "the",
        "ties", "of", "our", "common", "kindred", "to", "disavow", "these", "usurpations,", "which,", "would", "inevitably",
        "interrupt", "our", "connections", "and", "correspondence.", "They", "too", "have", "been", "deaf", "to", "the", "voice",
        "of", "justice", "and", "of", "consanguinity.", "We", "must,", "therefore,", "acquiesce", "in", "the", "necessity,", "which",
        "denounces", "our", "Separation,", "and", "hold", "them,", "as", "we", "hold", "the", "rest", "of", "mankind,", "Enemies", "in",
        "War,", "in", "Peace", "Friends.We,", "therefore,", "the", "Representatives", "of", "the", "united", "States", "of", "America,",
        "in", "General", "Congress,", "Assembled,", "appealing", "to", "the", "Supreme", "Judge", "of", "the", "world", "for", "the",
        "rectitude", "of", "our", "intentions,", "do,", "in", "the", "Name,", "and", "by", "Authority", "of", "the", "good", "People",
        "of", "these", "Colonies,", "solemnly", "publish", "and", "declare,", "That", "these", "United", "Colonies", "are,", "and",
        "of", "Right", "ought", "to", "be", "Free", "and", "Independent", "States;", "that", "they", "are", "Absolved", "from", "all",
        "Allegiance", "to", "the", "British", "Crown,", "and", "that", "all", "political", "connection", "between", "them", "and", "the",
        "State", "of", "Great", "Britain,", "is", "and", "ought", "to", "be", "totally", "dissolved;", "and", "that", "as", "Free", "and",
        "Independent", "States,", "they", "have", "full", "Power", "to", "levy", "War,", "conclude", "Peace,", "contract", "Alliances,",
        "establish", "Commerce,", "and", "to", "do", "all", "other", "Acts", "and", "Things", "which", "Independent", "States", "may",
        "of", "right", "do.", "And", "for", "the", "support", "of", "this", "Declaration,", "with", "a", "firm", "reliance", "on", "the",
        "protection", "of", "divine", "Providence,", "we", "mutually", "pledge", "to", "each", "other", "our", "Lives,", "our", "Fortunes",
        "and", "our", "sacred", "Honor."};


    for (auto it = input_set.begin(); it != input_set.end(); ++it) {
        m_table->key(*it);
    }
    std::set<std::string> output_set;
    bool is_in_done = false;
    bool is_out_done = false;
    size_t header_offset = 16;
    int count = 0;
    while (!is_in_done) {
        is_in_done = m_table->name_fill(header_offset);
        is_out_done = m_table->name_set(header_offset, output_set);
        header_offset = 0;
        ASSERT_EQ(is_in_done, is_out_done);
        ++count;
    }
    ASSERT_EQ(input_set, output_set);
    ASSERT_LT(1, count);
}
