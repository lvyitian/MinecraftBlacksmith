//
//  coreMods.cpp
//  MinecraftBlacksmith
//
//  Created by Hallowizer on 11/2/18.
//
//

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <list>
#include <string>

using namespace std;

#include <api/apiCoreMods.hpp>
#include "coreMods.hpp"
#include <patcher/patcher.hpp>

#define COREMOD_TYPE 1

typedef CoreMod (*cmInitFunc)(void);

static void registerTransformer(ModTransformer transformer);
static void discoverCoreMods(string);
static void processFile(string, string, string);
static void loadCoreMod(cmInitFunc);

static list<ModTransformer> transformers;

void setupCoreMods(string gameDir) {
    registerTransformer(patchTransform);
    
    discoverCoreMods(gameDir);
}

static void discoverCoreMods(string gameDir) {
    string modDir = gameDir + "mods/";
    
    DIR *dir = opendir(modDir.c_str());
    struct dirent *member;
    struct stat *sb = (struct stat *) malloc(sizeof(struct stat));
    
    if (dir) {
        while ((member = readdir(dir)) != NULL)
            if (stat((modDir + member->d_name).c_str(), sb) == 0 && S_ISREG(sb->st_mode))
                processFile(gameDir, modDir + member->d_name, member->d_name);
    }
    
    closedir(dir);
    
    free(sb);
}

static void processFile(string gameDir, string name, string singleName) {
    FILE *fp = fopen(name.c_str(), "r");
    
    char type;
    fscanf(fp, "%c", &type);
    
    if (type != COREMOD_TYPE)
        return;
    
    int len;
    fscanf(fp, "%d", &len);
    
    char *bytes = malloc(len);
    fread(bytes, 1, len, fp);
    
    fclose(fp);
    
    string tmpModDir = gameDir + "blacksmith/temp/mods/";
    fp = fopen((tmpModDir + singleName).c_str(), "r+");
    
    fprintf(fp, bytes);
    free(bytes);
    
    fclose(fp);
    remove((tmpModDir + singleName).c_str());
    
    cmInitFunc init = (cmInitFunc) loadSym(fp, "blacksmith_reserved_initCoreMod");
    loadCoreMod(init);
}

static void loadCoreMod(cmInitFunc init) {
    CoreMod coremod = init();

    ModTransformer *transformers = coremod.getTransformers();
    int transformerCount = coremod.getTransformerCount();

    int i;
    for (i = 0; i < transformerCount; i++)
    	registerTransformer(transformers[i]);
}

static void registerTransformer(ModTransformer transformer) {
    transformers.push_back(transformer);
}

void transformMod(ModBytecode& bytecode) {
    for (list<ModTransformer>::iterator iter = transformers.begin(); iter != transformers.end(); iter++)
        (*iter)(bytecode);
}
