// This is the header file for the things I'm not yet sure where to put.

char *get_bvfname_from_one_fafname(struct commet_job *settings, char *fafname) {
    char _bvfname[4096];
    sprintf(_bvfname, "%s/%s.bv", settings->output_directory, fafname);
    char *bvfname = calloc(strlen(_bvfname) + 1, sizeof(char));
    strcpy(bvfname, _bvfname);
    return bvfname;
}

char *get_bvfname_of_index_and_search(struct commet_job *settings, 
       char *ifafname, char *sfafname) {
    char _bvfname[4096];
    sprintf(_bvfname, "%s/%s_in_%s.bv", settings->output_directory, 
            sfafname, ifafname);
    char *bvfname = calloc(strlen(_bvfname) + 1, sizeof(char));
    strcpy(bvfname, _bvfname);
    return bvfname;
}
