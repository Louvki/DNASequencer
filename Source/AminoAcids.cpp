#include "AminoAcids.h"

namespace dna
{
const std::vector<AminoAcid>& getAminoAcids()
{
    static const std::vector<AminoAcid> aminoAcids
    {
        { "Ala", AminoAcidProperty::nonpolar,       { "GCT", "GCC", "GCA", "GCG" },                       0, 127, {}, "" },
        { "Arg", AminoAcidProperty::positiveCharge, { "CGT", "CGC", "CGA", "CGG", "AGA", "AGG" },         0, 127, {}, "" },
        { "Asn", AminoAcidProperty::uncharged,      { "AAT", "AAC" },                                      0, 127, {}, "" },
        { "Asp", AminoAcidProperty::negativeCharge, { "GAT", "GAC" },                                      0, 127, {}, "" },
        { "Cys", AminoAcidProperty::uncharged,      { "TGT", "TGC" },                                      0, 127, {}, "" },
        { "Gln", AminoAcidProperty::uncharged,      { "CAA", "CAG" },                                      0, 127, {}, "" },
        { "Glu", AminoAcidProperty::negativeCharge, { "GAA", "GAG" },                                      0, 127, {}, "" },
        { "Gly", AminoAcidProperty::nonpolar,       { "GGT", "GGC", "GGA", "GGG" },                       0, 127, {}, "" },
        { "His", AminoAcidProperty::positiveCharge, { "CAT", "CAC" },                                      0, 127, {}, "" },
        { "Ile", AminoAcidProperty::nonpolar,       { "ATT", "ATC", "ATA" },                               0, 127, {}, "" },
        { "Leu", AminoAcidProperty::nonpolar,       { "TTA", "TTG", "CTT", "CTC", "CTA", "CTG" },         0, 127, {}, "" },
        { "Lys", AminoAcidProperty::positiveCharge, { "AAA", "AAG" },                                      0, 127, {}, "" },
        { "Met", AminoAcidProperty::nonpolar,       { "ATG" },                                              0, 127, {}, "START" },
        { "Phe", AminoAcidProperty::nonpolar,       { "TTT", "TTC" },                                      0, 127, {}, "" },
        { "Pro", AminoAcidProperty::nonpolar,       { "CCT", "CCC", "CCA", "CCG" },                       0, 127, {}, "" },
        { "Ser", AminoAcidProperty::uncharged,      { "TCT", "TCC", "TCA", "TCG", "AGT", "AGC" },         0, 127, {}, "" },
        { "Thr", AminoAcidProperty::uncharged,      { "ACT", "ACC", "ACA", "ACG" },                       0, 127, {}, "" },
        { "Trp", AminoAcidProperty::nonpolar,       { "TGG" },                                              0, 127, {}, "" },
        { "Tyr", AminoAcidProperty::uncharged,      { "TAT", "TAC" },                                      0, 127, {}, "" },
        { "Val", AminoAcidProperty::nonpolar,       { "GTT", "GTC", "GTA", "GTG" },                       0, 127, {}, "" }
    };

    return aminoAcids;
}
}
