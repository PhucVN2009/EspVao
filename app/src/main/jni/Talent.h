#pragma once
#include "Image/bocpha_t.h"
#include "Image/capcuu_t.h"
#include "Image/gamthet_t.h"
#include "Image/lachansinhmenh_t.h"
#include "Image/ngatngu_t.h"
#include "Image/suynhuoc_t.h"
#include "Image/thanhtay_t.h"
#include "Image/tocbien_t.h"
#include "Image/tochanh_t.h"
#include "Image/trungtri_t.h"
#include "Image/tubaoton_t.h"
#include "Image/vebinh_t.h"

// Talent ID -> texture lookup (sparse, max 20 talents)
#define MAX_TALENTS 20
struct TalentEntry {
    unsigned int id;
    TextureInfo tex;
};
static TalentEntry g_talents[MAX_TALENTS] = {};
static int g_talentCount = 0;

static void LoadTalentImages() {
    g_talentCount = 0;
    g_talents[g_talentCount++] = {80108, createTexturePNGFromMem(bocpha_t, sizeof(bocpha_t))};
    g_talents[g_talentCount++] = {80102, createTexturePNGFromMem(capcuu_t, sizeof(capcuu_t))};
    g_talents[g_talentCount++] = {80110, createTexturePNGFromMem(gamthet_t, sizeof(gamthet_t))};
    g_talents[g_talentCount++] = {80504, createTexturePNGFromMem(lachansinhmenh_t, sizeof(lachansinhmenh_t))};
    g_talents[g_talentCount++] = {80103, createTexturePNGFromMem(ngatngu_t, sizeof(ngatngu_t))};
    g_talents[g_talentCount++] = {80105, createTexturePNGFromMem(suynhuoc_t, sizeof(suynhuoc_t))};
    g_talents[g_talentCount++] = {80107, createTexturePNGFromMem(thanhtay_t, sizeof(thanhtay_t))};
    g_talents[g_talentCount++] = {80115, createTexturePNGFromMem(tocbien_t, sizeof(tocbien_t))};
    g_talents[g_talentCount++] = {80109, createTexturePNGFromMem(tochanh_t, sizeof(tochanh_t))};
    g_talents[g_talentCount++] = {80104, createTexturePNGFromMem(trungtri_t, sizeof(trungtri_t))};
    g_talents[g_talentCount++] = {80503, createTexturePNGFromMem(tubaoton_t, sizeof(tubaoton_t))};
}

static TextureInfo GetTalentTexture(unsigned int talentId) {
    for (int i = 0; i < g_talentCount; i++) {
        if (g_talents[i].id == talentId) return g_talents[i].tex;
    }
    return {};
}
