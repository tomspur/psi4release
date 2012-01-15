import PsiMod
import re
import os
import input
import math
import warnings
import pickle
from driver import *
from molecule import *
from text import *
from collections import defaultdict

# Function to make calls among wrappers(), energy(), optimize(), etc.
def call_function_in_1st_argument(funcarg, **kwargs):
    return funcarg(**kwargs)

###################
##  Start of cp  ##
###################

def three_body(name, **kwargs):

    # Wrap any positional arguments into kwargs (for intercalls among wrappers)
    if not('name' in kwargs) and name:
        kwargs['name'] = name.lower()

    # Establish function to call
    if not('cp_func' in kwargs):
        if ('func' in kwargs):
            kwargs['cp_func'] = kwargs['func']
            del kwargs['func'] 
        else:
            kwargs['cp_func'] = energy
    func = kwargs['cp_func']
    if not func:
        raise ValidationError('Function \'%s\' does not exist to be called by wrapper counterpoise_correct.' % (func.__name__))
    if (func is db):
        raise ValidationError('Wrapper counterpoise_correct is unhappy to be calling function \'%s\'.' % (func.__name__))

    check_bsse = False
    if (kwargs.has_key('check_bsse')):
        check_bsse = kwargs['check_bsse']

    # Make sure the molecule the user provided is the active one
    if (kwargs.has_key('molecule')):
        activate(kwargs['molecule'])
        del kwargs['molecule']
    molecule = PsiMod.get_active_molecule()
    molecule.update_geometry()
    PsiMod.set_global_option("BASIS", PsiMod.get_global_option("BASIS"))

    ri_ints_io = PsiMod.get_option('DF_INTS_IO')
    PsiMod.set_global_option('DF_INTS_IO','SAVE')
    psioh = PsiMod.IOManager.shared_object()
    psioh.set_specific_retention(97, True)

    activate(molecule) 
    molecule.update_geometry()

    PsiMod.print_out("\n")
    banner("Three-Body Computation: Trimer ABC.\nFull Basis Set.")
    PsiMod.print_out("\n")
    e_trimer_full = []
    e_trimer_full.append(call_function_in_1st_argument(func, **kwargs))

    PsiMod.clean()  
    PsiMod.set_global_option('DF_INTS_IO','LOAD')

    # All dimers with ghosts
    dimers = extract_clusters(molecule, True, 2)
    if (len(dimers) != 3):
        print 'three_body: error, only trimers are allowed at present'

    e_dimer_full = []
    names = ['AB', 'AC', 'BC']
    for index in range(len(dimers)):
        cluster = dimers[index]
        activate(cluster)
        PsiMod.print_out("\n")
        banner(("Three-Body Computation: Dimer %s.\n Full Basis Set." % (names[index])))
        PsiMod.print_out("\n")
        e_dimer_full.append(call_function_in_1st_argument(func, **kwargs))
        PsiMod.clean()  

    # All monomers with ghosts
    monomers = extract_clusters(molecule, True, 1)
    e_monomer_full = []
    names = ['A', 'B', 'C']
    for index in range(len(monomers)):
        cluster = monomers[index]
        activate(cluster)
        PsiMod.print_out("\n")
        banner(("CP Computation: Monomer %s.\n Full Basis Set." % (names[index])))
        PsiMod.print_out("\n")
        e_monomer_full.append(call_function_in_1st_argument(func, **kwargs))
        PsiMod.clean()  

    PsiMod.set_global_option('DF_INTS_IO',ri_ints_io)
    psioh.set_specific_retention(97, False)

    activate(molecule) 

    # Energy analysis
    E_AB_A_B = e_dimer_full[0] - e_monomer_full[0] - e_monomer_full[1];
    E_AC_A_C = e_dimer_full[1] - e_monomer_full[0] - e_monomer_full[2];
    E_BC_B_C = e_dimer_full[2] - e_monomer_full[1] - e_monomer_full[2];

    E_ABC_A_B_C = e_trimer_full[0] - e_monomer_full[0] - e_monomer_full[1] - e_monomer_full[2]

    E_ABC_AB_AC_A = e_trimer_full[0] + e_monomer_full[0] - e_dimer_full[0] - e_dimer_full[1] 
    E_ABC_AB_BC_B = e_trimer_full[0] + e_monomer_full[1] - e_dimer_full[0] - e_dimer_full[2]
    E_ABC_AC_BC_C = e_trimer_full[0] + e_monomer_full[2] - e_dimer_full[1] - e_dimer_full[2]

    E_ABC_AB_AC_BC_A_B_C = e_trimer_full[0] + e_monomer_full[0] + e_monomer_full[1] + e_monomer_full[2] - e_dimer_full[0] - e_dimer_full[1] - e_dimer_full[2]

    PsiMod.print_out('\n\t==> Three-Body Computation Energy Results <==\n\n')

    PsiMod.print_out('    ABC =                %18.10E H %18.10E kcal mol^-1\n' % (e_trimer_full[0], 627.509 * e_trimer_full[0]))
    PsiMod.print_out('    AB =                 %18.10E H %18.10E kcal mol^-1\n' % (e_dimer_full[0], 627.509 * e_dimer_full[0]))
    PsiMod.print_out('    AC =                 %18.10E H %18.10E kcal mol^-1\n' % (e_dimer_full[1], 627.509 * e_dimer_full[1]))
    PsiMod.print_out('    BC =                 %18.10E H %18.10E kcal mol^-1\n' % (e_dimer_full[2], 627.509 * e_dimer_full[2]))
    PsiMod.print_out('    A =                  %18.10E H %18.10E kcal mol^-1\n' % (e_monomer_full[0], 627.509 * e_monomer_full[0]))
    PsiMod.print_out('    B =                  %18.10E H %18.10E kcal mol^-1\n' % (e_monomer_full[1], 627.509 * e_monomer_full[1]))
    PsiMod.print_out('    C =                  %18.10E H %18.10E kcal mol^-1\n' % (e_monomer_full[2], 627.509 * e_monomer_full[2]))
    PsiMod.print_out('    AB-A-B =             %18.10E H %18.10E kcal mol^-1\n' % (E_AB_A_B, 627.509 * E_AB_A_B))
    PsiMod.print_out('    AC-A-C =             %18.10E H %18.10E kcal mol^-1\n' % (E_AC_A_C, 627.509 * E_AC_A_C))
    PsiMod.print_out('    BC-B-C =             %18.10E H %18.10E kcal mol^-1\n' % (E_BC_B_C, 627.509 * E_BC_B_C))
    PsiMod.print_out('    ABC-A-B-C =          %18.10E H %18.10E kcal mol^-1\n' % (E_ABC_A_B_C, 627.509 * E_ABC_A_B_C))
    PsiMod.print_out('    ABC-AB-AC+A =        %18.10E H %18.10E kcal mol^-1\n' % (E_ABC_AB_AC_A, 627.509 * E_ABC_AB_AC_A))
    PsiMod.print_out('    ABC-AB-BC+B =        %18.10E H %18.10E kcal mol^-1\n' % (E_ABC_AB_BC_B, 627.509 * E_ABC_AB_BC_B))
    PsiMod.print_out('    ABC-AC-BC+C =        %18.10E H %18.10E kcal mol^-1\n' % (E_ABC_AC_BC_C, 627.509 * E_ABC_AC_BC_C))
    PsiMod.print_out('    ABC-AB-AC-BC+A+B+C = %18.10E H %18.10E kcal mol^-1\n' % (E_ABC_AB_AC_BC_A_B_C, 627.509 * E_ABC_AB_AC_BC_A_B_C))
        
    return 0.0 


def cp(name, **kwargs):

    # Wrap any positional arguments into kwargs (for intercalls among wrappers)
    if not('name' in kwargs) and name:
        kwargs['name'] = name.lower()

    # Establish function to call
    if not('cp_func' in kwargs):
        if ('func' in kwargs):
            kwargs['cp_func'] = kwargs['func']
            del kwargs['func'] 
        else:
            kwargs['cp_func'] = energy
    func = kwargs['cp_func']
    if not func:
        raise ValidationError('Function \'%s\' does not exist to be called by wrapper counterpoise_correct.' % (func.__name__))
    if (func is db):
        raise ValidationError('Wrapper counterpoise_correct is unhappy to be calling function \'%s\'.' % (func.__name__))

    check_bsse = False
    if (kwargs.has_key('check_bsse')):
        check_bsse = kwargs['check_bsse']

    # Make sure the molecule the user provided is the active one
    if (kwargs.has_key('molecule')):
        activate(kwargs['molecule'])
        del kwargs['molecule']
    molecule = PsiMod.get_active_molecule()
    molecule.update_geometry()
    PsiMod.set_global_option("BASIS", PsiMod.get_global_option("BASIS"))

    df_ints_io = PsiMod.get_option('DF_INTS_IO')
    PsiMod.set_global_option('DF_INTS_IO','SAVE')
    psioh = PsiMod.IOManager.shared_object()
    psioh.set_specific_retention(97, True)

    activate(molecule) 
    molecule.update_geometry()

    PsiMod.print_out("\n")
    banner("CP Computation: Complex.\nFull Basis Set.")
    PsiMod.print_out("\n")
    e_dimer = call_function_in_1st_argument(func, **kwargs)
    #e_dimer = energy(name, **kwargs)

    PsiMod.clean()  
    PsiMod.set_global_option('DF_INTS_IO','LOAD')

    # All monomers with ghosts
    monomers = extract_clusters(molecule, True, 1)
    e_monomer_full = []
    
    cluster_n = 0
    for cluster in monomers:
        activate(cluster)
        PsiMod.print_out("\n")
        banner(("CP Computation: Monomer %d.\n Full Basis Set." % (cluster_n + 1)))
        PsiMod.print_out("\n")
        e_monomer_full.append(call_function_in_1st_argument(func, **kwargs))
        #e_monomer_full.append(energy(name,**kwargs))
        cluster_n = cluster_n + 1
        PsiMod.clean()  

    PsiMod.set_global_option('DF_INTS_IO','NONE')
    if (check_bsse): 
        # All monomers without ghosts
        monomers = extract_clusters(molecule, False, 1)
        e_monomer_bsse = []
   
        cluster_n = 0
        for cluster in monomers:
            activate(cluster)
            PsiMod.print_out("\n")
            #cluster.print_to_output()
            banner(("CP Computation: Monomer %d.\n Monomer Set." % (cluster_n + 1)))
            PsiMod.print_out("\n")
            e_monomer_bsse.append(call_function_in_1st_argument(func, **kwargs))
            #e_monomer_bsse.append(energy(name,**kwargs))
            cluster_n = cluster_n + 1

    PsiMod.set_global_option('DF_INTS_IO',df_ints_io)
    psioh.set_specific_retention(97, False)

    activate(molecule) 
        
    if (check_bsse != True):
        cp_table = Table(rows = ["System:"], cols = ["Energy (full):"])
        cp_table["Complex"] = [e_dimer]
        for cluster_n in range(0,len(monomers)):
            key = "Monomer %d" % (cluster_n+1)
            cp_table[key] = [e_monomer_full[cluster_n]]
        
        e_full = e_dimer
        for cluster_n in range(0,len(monomers)):
            e_full = e_full - e_monomer_full[cluster_n]
        cp_table["Interaction"] = [e_full]
    
    else:
        cp_table = Table(rows = ["System:"], cols = ["Energy (full):","Energy (monomer):","BSSE:"])
        cp_table["Complex"] = [e_dimer, 0.0, 0.0]
        for cluster_n in range(0,len(monomers)):
            key = "Monomer %d" % (cluster_n+1)
            cp_table[key] = [e_monomer_full[cluster_n], e_monomer_bsse[cluster_n], \
                e_monomer_full[cluster_n] - e_monomer_bsse[cluster_n]]
        
        e_full = e_dimer
        e_bsse = e_dimer
        for cluster_n in range(0,len(monomers)):
            e_full = e_full - e_monomer_full[cluster_n]
            e_bsse = e_bsse - e_monomer_bsse[cluster_n]
        cp_table["Totals:"] = [e_full, e_bsse, e_full-e_bsse]
    
    PsiMod.print_out("\n")
    banner("CP Computation: Results.")
    PsiMod.print_out("\n")
    
    banner("Hartree",2)
    PsiMod.print_out("\n")
    
    PsiMod.print_out(str(cp_table))

    PsiMod.print_out("\n")
    banner("kcal*mol^-1",2)
    PsiMod.print_out("\n")

    cp_table.scale()   
 
    PsiMod.print_out(str(cp_table))
    return e_full

##  Aliases  ##
counterpoise_correct = cp
counterpoise_correction = cp

#################
##  End of cp  ##
#################



#########################
##  Start of Database  ##
#########################

def database(name, db_name, **kwargs):
    """Wrapper to access the molecule objects and reference energies of popular chemical databases.

    Required Arguments:
    -------------------
    * name (or unlabeled first argument) indicates the computational method to be applied to the database.
    * db_name (or unlabeled second argument) is a string of the requested database name. This name matches the
        python file in psi4/lib/databases , wherein literature citations for the database are also documented.

    Optional Arguments:  --> 'default_option' <-- 
    ------------------
    * mode = --> 'continuous' <-- | 'sow' | 'reap'
        Indicates whether the calculation required to complete the database are to be run in one
        file ('continuous') or are to be farmed out in an embarrassingly parallel fashion ('sow'/'reap').
        For the latter, run an initial job with 'sow' and follow instructions in its output file.
    * cp = 'on' | --> 'off' <--
        Indicates whether counterpoise correction is employed in computing interaction energies.
        Use this option and NOT the cp() wrapper for BSSE correction in the database() wrapper.
        Option valid only for databases consisting of bimolecular complexes.
    * rlxd = 'on' | --> 'off' <--
        Indicates whether correction for the deformation energy is employed in computing interaction energies.
        Option valid only for databases consisting of bimolecular complexes with non-frozen monomers.
    * symm = --> 'on' <-- | 'off'
        Indicates whether the native symmetry of the database molecules is employed ('on') or whether
        it is forced to c1 symmetry ('off'). Some computational methods (e.g., SAPT) require no symmetry,
        and this will be set by the database() wrapper.
    * zpe = 'on' | --> 'off' <--
        Indicates whether zero-point-energy corrections are appended to single-point energy values. Option
        valid only for certain thermochemical databases.
        Disabled until Hessians ready.
    * benchmark = --> 'default' <-- | etc.
        Indicates whether a non-default set of reference energies, if available, are employed for the 
        calculation of error statistics.
    * tabulate = --> [] <-- | etc.
        Indicates whether to form tables of variables other than the primary requested energy
    * subset
        Indicates a subset of the full database to run. This is a very flexible option and can used in
        three distinct ways, outlined below. Note that two take a string and the last takes a list.
        * subset = 'small' | 'large' | 'equilibrium'
            Calls predefined subsets of the requested database, either 'small', a few of the smallest
            database members, 'large', the largest of the database members, or 'equilibrium', the
            equilibrium geometries for a database composed of dissociation curves.
        * subset = 'BzBz_S' | 'FaOOFaON' | 'ArNe' | etc.
            For databases composed of dissociation curves, individual dissociation curves can be called
            by name. Consult the database python files for available molecular systems.
        * subset = [1,2,5] | ['1','2','5'] | ['BzMe-3.5', 'MeMe-5.0'] | etc.
            Specify a list of database members to run. Consult the database python files for available 
            molecular systems.
    """

    #hartree2kcalmol = 627.508924  # consistent with constants in physconst.h
    hartree2kcalmol = 627.509469  # consistent with perl SETS scripts 

    # Wrap any positional arguments into kwargs (for intercalls among wrappers)
    if not('name' in kwargs) and name:
        kwargs['name'] = name.lower()
    if not('db_name' in kwargs) and db_name:
        kwargs['db_name'] = db_name

    # Establish function to call
    if not('db_func' in kwargs):
        if ('func' in kwargs):
            kwargs['db_func'] = kwargs['func']
            del kwargs['func'] 
        else:
            kwargs['db_func'] = energy
    func = kwargs['db_func']
    if not func:
        raise ValidationError('Function \'%s\' does not exist to be called by wrapper database.' % (func.__name__))
    if (func is cp):
        raise ValidationError('Wrapper database is unhappy to be calling function \'%s\'. Use the cp keyword within database instead.' % (func.__name__))

    # Define path and load module for requested database
    sys.path.append('%sdatabases' % (PsiMod.Process.environment["PSIDATADIR"]))
    sys.path.append('%s/lib/databases' % PsiMod.psi_top_srcdir())
    try: 
        database = __import__(db_name)
    except ImportError: 
        PsiMod.print_out('\nPython module for database %s failed to load\n\n' % (db_name))
        PsiMod.print_out('\nSearch path that was tried:\n')
        PsiMod.print_out(", ".join(map(str, sys.path)))
        raise ValidationError("Python module loading problem for database " + str(db_name))
    else:
        dbse = database.dbse
        HRXN = database.HRXN
        ACTV = database.ACTV
        RXNM = database.RXNM
        BIND = database.BIND
        TAGL = database.TAGL
        GEOS = database.GEOS

    # Must collect (here) and set (below) basis sets after every new molecule activation
    user_basis = PsiMod.get_option('BASIS')
    user_df_basis_scf = PsiMod.get_option('DF_BASIS_SCF')
    user_df_basis_mp2 = PsiMod.get_option('DF_BASIS_MP2')
    user_df_basis_cc = PsiMod.get_option('DF_BASIS_CC')
    user_df_basis_sapt = PsiMod.get_option('DF_BASIS_SAPT')
    user_df_basis_elst = PsiMod.get_option('DF_BASIS_ELST')

    b_user_reference = PsiMod.has_global_option_changed('REFERENCE')
    user_reference = PsiMod.get_option('REFERENCE')
    user_memory = PsiMod.get_memory()

    user_molecule = PsiMod.get_active_molecule()

    # Configuration based upon e_name & db_name options
    #   Force non-supramolecular if needed
    symmetry_override = 0
    if re.match(r'^sapt', name, re.IGNORECASE):
        try:
            database.ACTV_SA
        except AttributeError:
            raise ValidationError('Database %s not suitable for non-supramolecular calculation.' % (db_name))
        else:
            symmetry_override = 1
            ACTV = database.ACTV_SA
    #   Force open-shell if needed
    openshell_override = 0
    if (user_reference == 'RHF') or (user_reference == 'RKS'):
        try:
            database.isOS
        except AttributeError:
            pass
        else:
            if input.yes.match(str(database.isOS)):
                openshell_override = 1
                PsiMod.print_out('\nSome reagents in database %s require an open-shell reference; will be reset to UHF/UKS as needed.\n' % (db_name))

    # Configuration based upon database keyword options
    #   Option symmetry- whether symmetry treated normally or turned off (currently req'd for dfmp2 & dft)
    db_symm = 'yes'
    if(kwargs.has_key('symm')):
        db_symm = kwargs['symm']

    if input.no.match(str(db_symm)):
        symmetry_override = 1
    elif input.yes.match(str(db_symm)):
        pass
    else:
        raise ValidationError('Symmetry mode \'%s\' not valid.' % (db_symm))

    #   Option mode of operation- whether db run in one job or files farmed out
    #db_mode = 'continuous'
    #if(kwargs.has_key('mode')):
    #    db_mode = kwargs['mode']
    if not('db_mode' in kwargs):
        if ('mode' in kwargs):
            kwargs['db_mode'] = kwargs['mode']
            del kwargs['mode']
        else:
            kwargs['db_mode'] = 'continuous'
    db_mode = kwargs['db_mode']

    if re.match(r'^continuous$', db_mode.lower()):
        pass
    elif re.match(r'^sow$', db_mode.lower()):
        pass
    elif re.match(r'^reap$', db_mode.lower()):
        if(kwargs.has_key('linkage')):
            db_linkage = kwargs['linkage']
        else:
            raise ValidationError('Database execution mode \'reap\' requires a linkage option.')
    else:
        raise ValidationError('Database execution mode \'%s\' not valid.' % (db_mode))

    #   Option counterpoise- whether for interaction energy databases run in bsse-corrected or not
    db_cp = 'no'
    if(kwargs.has_key('cp')):
        db_cp = kwargs['cp']

    if input.yes.match(str(db_cp)):
        try:
            database.ACTV_CP
        except AttributeError:
            raise ValidationError('Counterpoise correction mode \'yes\' invalid for database %s.' % (db_name))
        else:
            ACTV = database.ACTV_CP
    elif input.no.match(str(db_cp)):
        pass
    else:
        raise ValidationError('Counterpoise correction mode \'%s\' not valid.' % (db_cp))

    #   Option relaxed- whether for non-frozen-monomer interaction energy databases include deformation correction or not?
    db_rlxd = 'no'
    if(kwargs.has_key('rlxd')):
        db_rlxd = kwargs['rlxd']

    if input.yes.match(str(db_rlxd)):
        if input.yes.match(str(db_cp)):
            try:
                database.ACTV_CPRLX
                database.RXNM_CPRLX
            except AttributeError:
                raise ValidationError('Deformation and counterpoise correction mode \'yes\' invalid for database %s.' % (db_name))
            else:
                ACTV = database.ACTV_CPRLX
                RXNM = database.RXNM_CPRLX
        elif input.no.match(str(db_cp)):
            try:
                database.ACTV_RLX
            except AttributeError:
                raise ValidationError('Deformation correction mode \'yes\' invalid for database %s.' % (db_name))
            else:
                ACTV = database.ACTV_RLX
    elif input.no.match(str(db_rlxd)):
        pass
    else:
        raise ValidationError('Deformation correction mode \'%s\' not valid.' % (db_rlxd))

    #   Option zero-point-correction- whether for thermochem databases jobs are corrected by zpe
    db_zpe = 'no'
    if(kwargs.has_key('zpe')):
        db_zpe = kwargs['zpe']

    if input.yes.match(str(db_zpe)):
        raise ValidationError('Zero-point-correction mode \'yes\' not yet implemented.')
    elif input.no.match(str(db_zpe)):
        pass
    else:
        raise ValidationError('Zero-point-correction \'mode\' %s not valid.' % (db_zpe))

    #   Option benchmark- whether error statistics computed wrt alternate reference energies
    db_benchmark = 'default'
    if(kwargs.has_key('benchmark')):
        db_benchmark = kwargs['benchmark']

        if re.match(r'^default$', db_benchmark, re.IGNORECASE):
            pass
        else:
            try:
                getattr(database, 'BIND_' + db_benchmark)
            except AttributeError:
                raise ValidationError('Special benchmark \'%s\' not available for database %s.' % (db_benchmark, db_name))
            else:
                BIND = getattr(database, 'BIND_' + db_benchmark)

    #   Option tabulate- whether tables of variables other than primary energy method are formed
    db_tabulate = []
    if(kwargs.has_key('tabulate')):
        db_tabulate = kwargs['tabulate']

    #   Option subset- whether all of the database or just a portion is run
    db_subset = HRXN
    if(kwargs.has_key('subset')):
        db_subset = kwargs['subset']

    if isinstance(db_subset, basestring):
        if re.match(r'^small$', db_subset, re.IGNORECASE):
            try:
                database.HRXN_SM
            except AttributeError:
                raise ValidationError('Special subset \'small\' not available for database %s.' % (db_name))
            else:
                HRXN = database.HRXN_SM
        elif re.match(r'^large$', db_subset, re.IGNORECASE):
            try:
                database.HRXN_LG
            except AttributeError:
                raise ValidationError('Special subset \'large\' not available for database %s.' % (db_name))
            else:
                HRXN = database.HRXN_LG
        elif re.match(r'^equilibrium$', db_subset, re.IGNORECASE):
            try:
                database.HRXN_EQ
            except AttributeError:
                raise ValidationError('Special subset \'equilibrium\' not available for database %s.' % (db_name))
            else:
                HRXN = database.HRXN_EQ
        else:
            try:
                getattr(database, db_subset)
            except AttributeError:
                raise ValidationError('Special subset \'%s\' not available for database %s.' % (db_subset, db_name))
            else:
                HRXN = getattr(database, db_subset)
    else:
        temp = []
        for rxn in db_subset:
            if rxn in HRXN:
                temp.append(rxn)
            else:
                raise ValidationError('Subset element \'%s\' not a member of database %s.' % (str(rxn), db_name))
        HRXN = temp

    temp = []
    for rxn in HRXN:
        temp.append(ACTV['%s-%s' % (dbse, rxn)])
    HSYS = drop_duplicates(sum(temp, []))

    # Sow all the necessary reagent computations
    PsiMod.print_out("\n\n")
    banner(("Database %s Computation" % (db_name)))
    PsiMod.print_out("\n")

    #   write index of calcs to output file
    if re.match('continuous', db_mode.lower()):
        instructions  = """\n    The database single-job procedure has been selected through mode='continuous'.\n"""
        instructions +=   """    Calculations for the reagents will proceed in the order below and will be followed\n"""
        instructions +=   """    by summary results for the database.\n\n"""
        for rgt in HSYS:
            instructions += """                    %-s\n""" % (rgt)
        instructions += """\n    Alternatively, a farming-out of the database calculations may be accessed through\n"""
        instructions +=   """    the database wrapper option mode='sow'/'reap'.\n\n"""
        PsiMod.print_out(instructions)

    #   write sow/reap instructions and index of calcs to output file and reap input file
    if re.match('sow', db_mode.lower()):
        instructions  = """\n    The database sow/reap procedure has been selected through mode='sow'. In addition\n"""
        instructions +=   """    to this output file (which contains no quantum chemical calculations), this job\n"""
        instructions +=   """    has produced a number of input files (%s-*.in) for individual database members\n""" % (dbse)
        instructions +=   """    and a single input file (%s-master.in) with a database(mode='reap') command.\n""" % (dbse)
        instructions +=   """    The former may look very peculiar since processed and pickled python rather than\n"""
        instructions +=   """    raw input is written. Follow the instructions below to continue.\n\n"""
        instructions +=   """    (1)  Run all of the %s-*.in input files on any variety of computer architecture.\n""" % (dbse)
        instructions +=   """       The output file names must be as given below.\n\n"""
        for rgt in HSYS:
            instructions += """             psi4 -i %-27s -o %-27s\n""" % (rgt + '.in', rgt + '.out')
        instructions += """\n    (2)  Gather all the resulting output files in a directory. Place input file\n"""
        instructions +=   """         %s-master.in into that directory and run it. The job will be trivial in\n""" % (dbse)
        instructions +=   """         length and give summary results for the database in its output file.\n\n"""
        instructions +=   """             psi4 -i %-27s -o %-27s\n\n""" % (dbse + '-master.in', dbse + '-master.out')
        instructions +=   """    Alternatively, a single-job execution of the database may be accessed through\n"""
        instructions +=   """    the database wrapper option mode='continuous'.\n\n"""
        PsiMod.print_out(instructions)

        fmaster = open('%s-master.in' % (dbse), 'w')
        fmaster.write('# This is a psi4 input file auto-generated from the database() wrapper.\n\n')
        fmaster.write("database('%s', '%s', mode='reap', cp='%s', rlxd='%s', zpe='%s', benchmark='%s', linkage=%d, subset=%s, tabulate=%s)\n\n" %
            (name, db_name, db_cp, db_rlxd, db_zpe, db_benchmark, os.getpid(), HRXN, db_tabulate))
        fmaster.close()

    #   Loop through chemical systems
    ERGT = {}
    ERXN = {}
    VRGT = {}
    VRXN = {}
    for rgt in HSYS:
        VRGT[rgt] = {}

        # extra definition of molecule so that logic in building commands string has something to act on
        exec GEOS[rgt]
        molecule = PsiMod.get_active_molecule()

        # build string of title banner
        banners = ''
        banners += """PsiMod.print_out('\\n')\n"""
        banners += """banner(' Database %s Computation: Reagent %s \\n   %s')\n""" % (db_name, rgt, TAGL[rgt])
        banners += """PsiMod.print_out('\\n')\n\n"""

        # build string of lines that defines contribution of rgt to each rxn
        actives = ''
        actives += """PsiMod.print_out('   Database Contributions Map:\\n   %s\\n')\n""" % ('-' * 75)
        for rxn in HRXN:
            db_rxn = dbse + '-' + str(rxn)
            if rgt in ACTV[db_rxn]:
                actives += """PsiMod.print_out('   reagent %s contributes by %.4f to reaction %s\\n')\n""" \
                   % (rgt, RXNM[db_rxn][rgt], db_rxn)
        actives += """PsiMod.print_out('\\n')\n\n"""

        # build string of commands for options from the input file  TODO: handle local options too
        commands  = ''
        commands += """\nPsiMod.set_memory(%s)\n\n""" % (user_memory)
        for chgdopt in PsiMod.get_global_option_list():
            if PsiMod.has_option_changed(chgdopt):
                chgdoptval = PsiMod.get_global_option(chgdopt)
                #chgdoptval = PsiMod.get_option(chgdopt)
                if isinstance(chgdoptval, basestring):
                    commands += """PsiMod.set_global_option('%s', '%s')\n""" % (chgdopt, chgdoptval)
                elif isinstance(chgdoptval, int) or isinstance(chgdoptval, float):
                    commands += """PsiMod.set_global_option('%s', %s)\n""" % (chgdopt, chgdoptval)
                else:
                    raise ValidationError('Option \'%s\' is not of a type (string, int, float, bool) that can be processed by database wrapper.' % (chgdopt))

        # build string of molecule and commands that are dependent on the database
        commands += '\n'
        commands += """PsiMod.set_global_option('BASIS', '%s')\n""" % (user_basis)
        if not((user_df_basis_scf == "") or (user_df_basis_scf == 'NONE')):
            commands += """PsiMod.set_global_option('DF_BASIS_SCF', '%s')\n""" % (user_df_basis_scf)
        if not((user_df_basis_mp2 == "") or (user_df_basis_mp2 == 'NONE')):
            commands += """PsiMod.set_global_option('DF_BASIS_MP2', '%s')\n""" % (user_df_basis_mp2)
        if not((user_df_basis_cc == "") or (user_df_basis_cc == 'NONE')):
            commands += """PsiMod.set_global_option('DF_BASIS_CC', '%s')\n""" % (user_df_basis_cc)
        if not((user_df_basis_sapt == "") or (user_df_basis_sapt == 'NONE')):
            commands += """PsiMod.set_global_option('DF_BASIS_SAPT', '%s')\n""" % (user_df_basis_sapt)
        if not((user_df_basis_elst == "") or (user_df_basis_elst == 'NONE')):
            commands += """PsiMod.set_global_option('DF_BASIS_ELST', '%s')\n""" % (user_df_basis_elst)
        commands += """molecule = PsiMod.get_active_molecule()\n"""
        commands += """molecule.update_geometry()\n"""

        if symmetry_override:
            commands += """molecule.reset_point_group('c1')\n"""
            commands += """molecule.fix_orientation(1)\n"""
            commands += """molecule.update_geometry()\n"""

        if (openshell_override) and (molecule.multiplicity() != 1):
            if user_reference == 'RHF': 
                commands += """PsiMod.set_global_option('REFERENCE', 'UHF')\n"""
            elif user_reference == 'RKS': 
                commands += """PsiMod.set_global_option('REFERENCE', 'UKS')\n"""

        # all modes need to step through the reagents but all for different purposes
        # continuous: defines necessary commands, executes energy(method) call, and collects results into dictionary
        # sow: opens individual reagent input file, writes the necessary commands, and writes energy(method) call
        # reap: opens individual reagent output file, collects results into a dictionary
        if re.match('continuous', db_mode.lower()):
            exec banners
            exec GEOS[rgt]
            exec commands
            #print 'MOLECULE LIVES %23s %8s %4d %4d %4s' % (rgt, PsiMod.get_option('REFERENCE'),
            #    molecule.molecular_charge(), molecule.multiplicity(), molecule.schoenflies_symbol())
            PsiMod.set_variable('NATOM', molecule.natom())
            ERGT[rgt] = call_function_in_1st_argument(func, **kwargs)
            #print ERGT[rgt]
            PsiMod.print_variables()
            exec actives
            for envv in db_tabulate:
               VRGT[rgt][envv] = PsiMod.get_variable(envv)
            PsiMod.set_global_option("REFERENCE", user_reference)
            PsiMod.clean()
            
        elif re.match('sow', db_mode.lower()):
            freagent = open('%s.in' % (rgt), 'w')
            freagent.write('# This is a psi4 input file auto-generated from the database() wrapper.\n\n')
            freagent.write(banners)
            freagent.write(GEOS[rgt])
            freagent.write(commands)
            freagent.write('''\npickle_kw = ("""''')
            pickle.dump(kwargs, freagent)
            freagent.write('''""")\n''')
            freagent.write("""\nkwargs = pickle.loads(pickle_kw)\n""")
            freagent.write("""electronic_energy = %s(**kwargs)\n\n""" % (func.__name__))
            freagent.write("""PsiMod.print_variables()\n""")
            freagent.write("""PsiMod.print_out('\\nDATABASE RESULT: computation %d for reagent %s """
                % (os.getpid(), rgt))
            freagent.write("""yields electronic energy %20.12f\\n' % (electronic_energy))\n\n""")
            freagent.write("""PsiMod.set_variable('NATOM', molecule.natom())\n""")
            for envv in db_tabulate:
                freagent.write("""PsiMod.print_out('DATABASE RESULT: computation %d for reagent %s """
                    % (os.getpid(), rgt))
                freagent.write("""yields variable value    %20.12f for variable %s\\n' % (PsiMod.get_variable(""")
                freagent.write("""'%s'), '%s'))\n""" % (envv.upper(), envv.upper()))
            freagent.close()

        elif re.match('reap', db_mode.lower()):
            ERGT[rgt] = 0.0
            for envv in db_tabulate:
               VRGT[rgt][envv] = 0.0
            exec banners
            exec actives
            try:
                freagent = open('%s.out' % (rgt), 'r')
            except IOError:
                PsiMod.print_out('Warning: Output file \'%s.out\' not found.\n' % (rgt))
                PsiMod.print_out('         Database summary will have 0.0 and **** in its place.\n')
            else:
                while 1:
                    line = freagent.readline()
                    if not line:
                        if ERGT[rgt] == 0.0:
                           PsiMod.print_out('Warning: Output file \'%s.out\' has no DATABASE RESULT line.\n' % (rgt))
                           PsiMod.print_out('         Database summary will have 0.0 and **** in its place.\n')
                        break
                    s = line.split()
                    if (len(s) != 0) and (s[0:3] == ['DATABASE', 'RESULT:', 'computation']):
                        if int(s[3]) != db_linkage:
                           raise ValidationError('Output file \'%s.out\' has linkage %s incompatible with master.in linkage %s.' 
                               % (rgt, str(s[3]), str(db_linkage)))
                        if s[6] != rgt:
                           raise ValidationError('Output file \'%s.out\' has nominal affiliation %s incompatible with reagent %s.' 
                               % (rgt, s[6], rgt))
                        if (s[8:10] == ['electronic', 'energy']):
                            ERGT[rgt] = float(s[10])
                            PsiMod.print_out('DATABASE RESULT: electronic energy = %20.12f\n' % (ERGT[rgt]))
                        elif (s[8:10] == ['variable', 'value']):
                            for envv in db_tabulate:
                                if (s[13:] == envv.upper().split()):
                                    VRGT[rgt][envv] = float(s[10])
                                    PsiMod.print_out('DATABASE RESULT: variable %s value    = %20.12f\n' % (envv.upper(), VRGT[rgt][envv]))
                freagent.close()

    #   end sow after writing files 
    if re.match('sow', db_mode.lower()):
        return 0.0

    # Reap all the necessary reaction computations
    PsiMod.print_out("\n")
    banner(("Database %s Results" % (db_name)))
    PsiMod.print_out("\n")

    maxactv = []
    for rxn in HRXN:
        maxactv.append(len(ACTV[dbse+'-'+str(rxn)]))
    maxrgt = max(maxactv) 
    table_delimit = '-' * (54+20*maxrgt)
    tables = ''

    #   find any reactions that are incomplete
    FAIL = defaultdict(int)
    for rxn in HRXN:
        db_rxn = dbse + '-' + str(rxn)
        for i in range(len(ACTV[db_rxn])):
            if abs(ERGT[ACTV[db_rxn][i]]) < 1.0e-12:
                FAIL[rxn] = 1

    #   tabulate requested process::environment variables
    tables += """   For each VARIABLE requested by tabulate, a 'Reaction Value' will be formed from\n"""
    tables += """   'Reagent' values according to weightings 'Wt', as for the REQUESTED ENERGY below.\n"""
    tables += """   Depending on the nature of the variable, this may or may not make any physical sense.\n"""
    for envv in db_tabulate:
        tables += """\n   ==> %s <==\n\n""" % (envv.title())
        tables += tblhead(maxrgt, table_delimit, 2)

        for rxn in HRXN:
            db_rxn = dbse + '-' + str(rxn)
            VRXN[db_rxn] = {}
    
            if FAIL[rxn]:
                tables += """\n%23s   %8s %8s   %8s""" % (db_rxn, '', '****', '')
                for i in range(len(ACTV[db_rxn])):
                    tables += """ %16.8f %2.0f""" % (VRGT[ACTV[db_rxn][i]][envv], RXNM[db_rxn][ACTV[db_rxn][i]])
    
            else:
                VRXN[db_rxn][envv] = 0.0
                for i in range(len(ACTV[db_rxn])):
                    VRXN[db_rxn][envv] += VRGT[ACTV[db_rxn][i]][envv] * RXNM[db_rxn][ACTV[db_rxn][i]]
            
                tables += """\n%23s        %16.8f       """ % (db_rxn, VRXN[db_rxn][envv])
                for i in range(len(ACTV[db_rxn])):
                    tables += """ %16.8f %2.0f""" % (VRGT[ACTV[db_rxn][i]][envv], RXNM[db_rxn][ACTV[db_rxn][i]])
        tables += """\n   %s\n""" % (table_delimit)

    #   tabulate primary requested energy variable with statistics
    count_rxn = 0
    minDerror = 100000.0
    maxDerror = 0.0
    MSDerror  = 0.0
    MADerror  = 0.0
    RMSDerror = 0.0

    tables += """\n   ==> %s <==\n\n""" % ('Requested Energy')
    tables += tblhead(maxrgt, table_delimit, 1)
    for rxn in HRXN:
        db_rxn = dbse + '-' + str(rxn)

        if FAIL[rxn]:
            tables += """\n%23s   %8.4f %8s   %8s""" % (db_rxn, BIND[db_rxn], '****', '****')
            for i in range(len(ACTV[db_rxn])):
                tables += """ %16.8f %2.0f""" % (ERGT[ACTV[db_rxn][i]], RXNM[db_rxn][ACTV[db_rxn][i]])

        else:
            ERXN[db_rxn] = 0.0
            for i in range(len(ACTV[db_rxn])):
                ERXN[db_rxn] += ERGT[ACTV[db_rxn][i]] * RXNM[db_rxn][ACTV[db_rxn][i]]
            error = hartree2kcalmol * ERXN[db_rxn] - BIND[db_rxn]
        
            tables += """\n%23s   %8.4f %8.4f   %8.4f""" % (db_rxn, BIND[db_rxn], hartree2kcalmol*ERXN[db_rxn], error)
            for i in range(len(ACTV[db_rxn])):
                tables += """ %16.8f %2.0f""" % (ERGT[ACTV[db_rxn][i]], RXNM[db_rxn][ACTV[db_rxn][i]])

            if abs(error) < abs(minDerror): minDerror = error
            if abs(error) > abs(maxDerror): maxDerror = error
            MSDerror += error
            MADerror += abs(error)
            RMSDerror += error*error
            count_rxn += 1
    tables += """\n   %s\n""" % (table_delimit)

    if count_rxn:

        MSDerror  /= float(count_rxn)
        MADerror  /= float(count_rxn)
        RMSDerror  = sqrt(RMSDerror/float(count_rxn))

        tables += """%23s   %19s %8.4f\n""" % ('Minimal Dev', '', minDerror)
        tables += """%23s   %19s %8.4f\n""" % ('Maximal Dev', '', maxDerror)
        tables += """%23s   %19s %8.4f\n""" % ('Mean Signed Dev', '', MSDerror)
        tables += """%23s   %19s %8.4f\n""" % ('Mean Absolute Dev', '', MADerror)
        tables += """%23s   %19s %8.4f\n""" % ('RMS Dev', '', RMSDerror)
        tables += """   %s\n""" % (table_delimit)

        PsiMod.set_variable('%s DATABASE MEAN SIGNED DEVIATION' % (db_name), MSDerror)
        PsiMod.set_variable('%s DATABASE MEAN ABSOLUTE DEVIATION' % (db_name), MADerror)
        PsiMod.set_variable('%s DATABASE ROOT-MEAN-SQUARE DEVIATION' % (db_name), RMSDerror)

        #print tables
        PsiMod.print_out(tables)
        finalenergy = MADerror

    else:
        finalenergy = 0.0

    # restore molecule and options
    activate(user_molecule) 
    user_molecule.update_geometry()
    PsiMod.set_global_option("BASIS", user_basis)
    PsiMod.set_global_option("REFERENCE", user_reference)
    if not b_user_reference:
        PsiMod.revoke_global_option_changed('REFERENCE')

    return finalenergy

def drop_duplicates(seq): 
    noDupes = []
    [noDupes.append(i) for i in seq if not noDupes.count(i)]
    return noDupes

def tblhead(tbl_maxrgt, tbl_delimit, ttype):
    tbl_str = ''
    tbl_str += """   %s""" % (tbl_delimit)
    if   ttype == 1: tbl_str += """\n%23s %19s   %8s""" % ('Reaction', 'Reaction Energy', 'Error')
    elif ttype == 2: tbl_str += """\n%23s     %19s %6s""" % ('Reaction', 'Reaction Value', '')
    for i in range(tbl_maxrgt):
        tbl_str += """%20s""" % ('Reagent '+str(i+1))
    if   ttype == 1: tbl_str += """\n%23s   %8s %8s %8s""" % ('', 'Ref', 'Calc', '[kcal/mol]')
    elif ttype == 2: tbl_str += """\n%54s""" % ('')
    for i in range(tbl_maxrgt):
        if   ttype == 1: tbl_str += """%20s""" % ('[H] Wt')
        elif ttype == 2: tbl_str += """%20s""" % ('Value Wt')
    tbl_str += """\n   %s""" % (tbl_delimit)
    return tbl_str

##  Aliases  ##
db = database

#######################
##  End of Database  ##
#######################



###################################
##  Start of Complete Basis Set  ##
###################################

def complete_basis_set(name, **kwargs):
    """Wrapper to define an energy method with basis set extrapolations and delta corrections.

    A CBS energy method is defined in four sequential stages (scf, corl, delta, delta2) covering 
    treatment of the reference total energy, the correlation energy, a delta correction to the 
    correlation energy, and a second delta correction. Each is activated by its stage_wfn keyword 
    and is only allowed if all preceding stages are active.

    Required Arguments:
    -------------------
    * name (or unlabeled first argument) indicates the computational method for the correlation energy, unless
        only reference step to be performed, in which case should be 'scf'. May be overruled if wfn keywords given.

    Optional Arguments:  --> 'default_option' <-- 
    ------------------
    Energy Methods:  Indicates the energy method employed for each stage
    * corl_wfn          = 'mp2' | 'ccsd(t)' | etc.
    * delta_wfn         = 'ccsd' | 'ccsd(t)' | etc.
    * delta_wfn_lesser  = --> 'mp2' <-- | 'ccsd' | etc.
    * delta2_wfn        = 'ccsd' | 'ccsd(t)' | etc.
    * delta2_wfn_lesser = --> 'mp2' <-- | 'ccsd(t)' | etc.

    Basis Sets:  Indicates the single or sequence of basis sets employed for each stage.
    * scf_basis    = --> corl_basis <-- | 'cc-pV[TQ]Z' | 'jun-cc-pv[tq5]z' | '6-31G*' | etc.
    * corl_basis   = 'cc-pV[TQ]Z' | 'jun-cc-pv[tq5]z' | '6-31G*' | etc.
    * delta_basis  = 'cc-pV[TQ]Z' | 'jun-cc-pv[tq5]z' | '6-31G*' | etc.
    * delta2_basis = 'cc-pV[TQ]Z' | 'jun-cc-pv[tq5]z' | '6-31G*' | etc.

    Schemes:  Indicates the formula for performing basis set extrapolation (or using the single best
        basis with highest_1) for each stage.
    * scf_scheme    = --> highest_1 <-- | scf_xtpl_helgaker_3
    * corl_scheme   = --> highest_1 <-- | corl_xtpl_helgaker_2
    * delta_scheme  = --> highest_1 <-- | corl_xtpl_helgaker_2
    * delta2_scheme = --> highest_1 <-- | corl_xtpl_helgaker_2
    """

    # Wrap any positional arguments into kwargs (for intercalls among wrappers)
    if not('name' in kwargs) and name:
        kwargs['name'] = name.lower()

    # Establish function to call (only energy makes sense for cbs)
    if not('cbs_func' in kwargs):
        if ('func' in kwargs):
            kwargs['cbs_func'] = kwargs['func']
            del kwargs['func'] 
        else:
            kwargs['cbs_func'] = energy
    func = kwargs['cbs_func']
    if not func:
        raise ValidationError('Function \'%s\' does not exist to be called by wrapper complete_basis_set.' % (func.__name__))
    if not(func is energy):
        raise ValidationError('Wrapper complete_basis_set is unhappy to be calling function \'%s\' instead of \'energy\'.' % (func.__name__))

    # Define some quantum chemical knowledge, namely what methods are subsumed in others
    VARH = {}
    VARH['scf']     = {'scftot'      : 'SCF TOTAL ENERGY'          }
    VARH['mp2']     = {'scftot'      : 'SCF TOTAL ENERGY',
                       'mp2corl'     : 'MP2 CORRELATION ENERGY'    }
    VARH['ccsd']    = {'scftot'      : 'SCF TOTAL ENERGY',
                       'mp2corl'     : 'MP2 CORRELATION ENERGY',
                       'ccsdcorl'    : 'CCSD CORRELATION ENERGY'   }
    VARH['ccsd(t)'] = {'scftot'      : 'SCF TOTAL ENERGY',
                       'mp2corl'     : 'MP2 CORRELATION ENERGY',
                       'ccsdcorl'    : 'CCSD CORRELATION ENERGY',
                       'ccsd(t)corl' : 'CCSD(T) CORRELATION ENERGY'}

    finalenergy = 0.0
    do_scf = 1
    do_corl = 0
    do_delta = 0
    do_delta2 = 0

    # Must collect (here) and set (below) basis sets after every new molecule activation
    b_user_basis = PsiMod.has_global_option_changed('BASIS')
    user_basis = PsiMod.get_option('BASIS')
    #user_df_basis_scf = PsiMod.get_option('DF_BASIS_SCF')
    #user_df_basis_mp2 = PsiMod.get_option('DF_BASIS_MP2')
    #user_df_basis_cc = PsiMod.get_option('DF_BASIS_CC')
    #user_df_basis_sapt = PsiMod.get_option('DF_BASIS_SAPT')
    #user_df_basis_elst = PsiMod.get_option('DF_BASIS_ELST')
    b_user_wfn = PsiMod.has_global_option_changed('WFN')
    user_wfn = PsiMod.get_option('WFN')

    # Make sure the molecule the user provided is the active one
    if (kwargs.has_key('molecule')):
        activate(kwargs['molecule'])
        del kwargs['molecule']
    molecule = PsiMod.get_active_molecule()
    molecule.update_geometry()
    PsiMod.set_global_option("BASIS", PsiMod.get_global_option("BASIS"))

    # Establish method for correlation energy
    if (kwargs.has_key('name')):
        if re.match(r'^scf$', kwargs['name'].lower()):
            pass
        else:
            do_corl = 1
            cbs_corl_wfn = kwargs['name'].lower()
    if (kwargs.has_key('corl_wfn')):
        do_corl = 1
        cbs_corl_wfn = kwargs['corl_wfn'].lower()
    if do_corl:
        if not (cbs_corl_wfn in VARH.keys()):
            raise ValidationError('Requested CORL method \'%s\' is not recognized. Add it to VARH in wrapper.py to proceed.' % (cbs_corl_wfn))

    # Establish method for delta correction energy
    if (kwargs.has_key('delta_wfn')):
        do_delta = 1
        cbs_delta_wfn = kwargs['delta_wfn'].lower()
        if not (cbs_delta_wfn in VARH.keys()):
            raise ValidationError('Requested DELTA method \'%s\' is not recognized. Add it to VARH in wrapper.py to proceed.' % (cbs_delta_wfn))

        if (kwargs.has_key('delta_wfn_lesser')):
            cbs_delta_wfn_lesser = kwargs['delta_wfn_lesser'].lower()
        else:
            cbs_delta_wfn_lesser = 'mp2'
        if not (cbs_delta_wfn_lesser in VARH.keys()):
            raise ValidationError('Requested DELTA method lesser \'%s\' is not recognized. Add it to VARH in wrapper.py to proceed.' % (cbs_delta_wfn_lesser))

    # Establish method for second delta correction energy
    if (kwargs.has_key('delta2_wfn')):
        do_delta2 = 1
        cbs_delta2_wfn = kwargs['delta2_wfn'].lower()
        if not (cbs_delta2_wfn in VARH.keys()):
            raise ValidationError('Requested DELTA2 method \'%s\' is not recognized. Add it to VARH in wrapper.py to proceed.' % (cbs_delta2_wfn))

        if (kwargs.has_key('delta2_wfn_lesser')):
            cbs_delta2_wfn_lesser = kwargs['delta2_wfn_lesser'].lower()
        else:
            cbs_delta2_wfn_lesser = 'mp2'
        if not (cbs_delta2_wfn_lesser in VARH.keys()):
            raise ValidationError('Requested DELTA2 method lesser \'%s\' is not recognized. Add it to VARH in wrapper.py to proceed.' % (cbs_delta2_wfn_lesser))

    # Check that user isn't skipping steps in scf + corl + delta + delta2 sequence
    if do_scf and not do_corl and not do_delta and not do_delta2:
        pass
    elif do_scf and do_corl and not do_delta and not do_delta2:
        pass
    elif do_scf and do_corl and do_delta and not do_delta2:
        pass
    elif do_scf and do_corl and do_delta and do_delta2:
        pass
    else:
        raise ValidationError('Requested scf (%s) + corl (%s) + delta (%s) + delta2 (%s) not valid. These steps are cummulative.' %
            (do_scf, do_corl, do_delta, do_delta2))

    # Establish list of valid basis sets for correlation energy
    if do_corl:
        if(kwargs.has_key('corl_basis')):
            BSTC, ZETC = validate_bracketed_basis(kwargs['corl_basis'].lower())
        else:
            raise ValidationError('CORL basis sets through keyword \'%s\' are required.' % ('corl_basis'))

    # Establish list of valid basis sets for scf energy
    if(kwargs.has_key('scf_basis')):
        BSTR, ZETR = validate_bracketed_basis(kwargs['scf_basis'].lower())
    else:
        if do_corl:
            BSTR = BSTC[:]
            ZETR = ZETC[:]
        else:
            raise ValidationError('SCF basis sets through keyword \'%s\' are required. Or perhaps you forgot the \'%s\'.' % ('scf_basis', 'corl_wfn'))

    # Establish list of valid basis sets for delta correction energy
    if do_delta:
        if(kwargs.has_key('delta_basis')):
            BSTD, ZETD = validate_bracketed_basis(kwargs['delta_basis'].lower())
        else:
            raise ValidationError('DELTA basis sets through keyword \'%s\' are required.' % ('delta_basis'))

    # Establish list of valid basis sets for second delta correction energy
    if do_delta2:
        if(kwargs.has_key('delta2_basis')):
            BSTD2, ZETD2 = validate_bracketed_basis(kwargs['delta2_basis'].lower())
        else:
            raise ValidationError('DELTA2 basis sets through keyword \'%s\' are required.' % ('delta2_basis'))

    # Establish treatment for scf energy (validity check useless since python will catch it long before here)
    cbs_scf_scheme = highest_1
    if(kwargs.has_key('scf_scheme')):
        cbs_scf_scheme = kwargs['scf_scheme']

    # Establish treatment for correlation energy
    cbs_corl_scheme = highest_1
    if(kwargs.has_key('corl_scheme')):
        cbs_corl_scheme = kwargs['corl_scheme']

    # Establish treatment for delta correction energy
    cbs_delta_scheme = highest_1
    if(kwargs.has_key('delta_scheme')):
        cbs_delta_scheme = kwargs['delta_scheme']

    # Establish treatment for delta2 correction energy
    cbs_delta2_scheme = highest_1
    if(kwargs.has_key('delta2_scheme')):
        cbs_delta2_scheme = kwargs['delta2_scheme']

    # Build string of title banner
    cbsbanners = ''
    cbsbanners += """PsiMod.print_out('\\n')\n"""
    cbsbanners += """banner(' CBS Setup ')\n"""
    cbsbanners += """PsiMod.print_out('\\n')\n\n"""
    exec cbsbanners

    # Call schemes for each portion of total energy to 'place orders' for calculations needed
    d_fields = ['d_stage', 'd_scheme', 'd_basis', 'd_wfn', 'd_need', 'd_coef', 'd_energy']
    f_fields = ['f_wfn', 'f_portion', 'f_basis', 'f_zeta', 'f_energy']
    GRAND_NEED = []
    MODELCHEM = []
    bstring = ''
    if do_scf:
        NEED = call_function_in_1st_argument(cbs_scf_scheme, 
            mode='requisition', basisname=BSTR, basiszeta=ZETR, wfnname='scf')
        GRAND_NEED.append(dict(zip(d_fields, ['scf', cbs_scf_scheme, reconstitute_bracketed_basis(NEED), 'scf', NEED, +1, 0.0])))

    if do_corl:
        NEED = call_function_in_1st_argument(cbs_corl_scheme, 
            mode='requisition', basisname=BSTC, basiszeta=ZETC, wfnname=cbs_corl_wfn)
        GRAND_NEED.append(dict(zip(d_fields, ['corl', cbs_corl_scheme, reconstitute_bracketed_basis(NEED), cbs_corl_wfn, NEED, +1, 0.0])))

    if do_delta:
        NEED = call_function_in_1st_argument(cbs_delta_scheme, 
            mode='requisition', basisname=BSTD, basiszeta=ZETD, wfnname=cbs_delta_wfn)
        GRAND_NEED.append(dict(zip(d_fields, ['delta', cbs_delta_scheme, reconstitute_bracketed_basis(NEED), cbs_delta_wfn, NEED, +1, 0.0])))

        NEED = call_function_in_1st_argument(cbs_delta_scheme, 
            mode='requisition', basisname=BSTD, basiszeta=ZETD, wfnname=cbs_delta_wfn_lesser)
        GRAND_NEED.append(dict(zip(d_fields, ['delta', cbs_delta_scheme, reconstitute_bracketed_basis(NEED), cbs_delta_wfn_lesser, NEED, -1, 0.0])))

    if do_delta2:
        NEED = call_function_in_1st_argument(cbs_delta2_scheme, 
            mode='requisition', basisname=BSTD2, basiszeta=ZETD2, wfnname=cbs_delta2_wfn)
        GRAND_NEED.append(dict(zip(d_fields, ['delta2', cbs_delta2_scheme, reconstitute_bracketed_basis(NEED), cbs_delta2_wfn, NEED, +1, 0.0])))

        NEED = call_function_in_1st_argument(cbs_delta2_scheme, 
            mode='requisition', basisname=BSTD2, basiszeta=ZETD2, wfnname=cbs_delta2_wfn_lesser)
        GRAND_NEED.append(dict(zip(d_fields, ['delta2', cbs_delta2_scheme, reconstitute_bracketed_basis(NEED), cbs_delta2_wfn_lesser, NEED, -1, 0.0])))

    for stage in GRAND_NEED:
        for lvl in stage['d_need'].iteritems():
            MODELCHEM.append(lvl[1])

    # Apply chemical reasoning to choose the minimum computations to run
    JOBS = MODELCHEM[:]

    instructions  = ''
    instructions += """    Naive listing of computations required.\n"""
    for mc in JOBS:
        instructions += """   %12s / %-24s for  %s\n""" % (mc['f_wfn'], mc['f_basis'], VARH[mc['f_wfn']][mc['f_wfn']+mc['f_portion']])

    #     Remove duplicate modelchem portion listings
    for indx_mc, mc in enumerate(MODELCHEM):
        dups = -1
        for indx_job, job in enumerate(JOBS):
            if (job['f_wfn'] == mc['f_wfn']) and (job['f_basis'] == mc['f_basis']):
                dups += 1
                if (dups >= 1):
                    del JOBS[indx_job]

    #     Remove chemically subsumed modelchem portion listings
    for indx_mc, mc in enumerate(MODELCHEM):
        for menial in VARH[mc['f_wfn']]:
            for indx_job, job in enumerate(JOBS):
                if (menial == job['f_wfn']+job['f_portion']) and (mc['f_basis'] == job['f_basis']) and not (mc['f_wfn'] == job['f_wfn']):
                    del JOBS[indx_job]

    instructions += """\n    Enlightened listing of computations required.\n"""
    for mc in JOBS:
        instructions += """   %12s / %-24s for  %s\n""" % (mc['f_wfn'], mc['f_basis'], VARH[mc['f_wfn']][mc['f_wfn']+mc['f_portion']])

    #     Expand listings to all that will be obtained
    JOBS_EXT = []
    for indx_job, job in enumerate(JOBS):
        for menial in VARH[job['f_wfn']]:
            temp_wfn, temp_portion = split_menial(menial)
            JOBS_EXT.append(dict(zip(f_fields, [temp_wfn, temp_portion, job['f_basis'], job['f_zeta'], 0.0])))

    #instructions += """\n    Full listing of computations to be obtained (required and bonus).\n"""
    #for mc in JOBS_EXT:
    #    instructions += """   %12s / %-24s for  %s\n""" % (mc['f_wfn'], mc['f_basis'], VARH[mc['f_wfn']][mc['f_wfn']+mc['f_portion']])
    PsiMod.print_out(instructions)


    # Run necessary computations
    for mc in JOBS:
        kwargs['name'] = mc['f_wfn']

        # Build string of title banner
        cbsbanners = ''
        cbsbanners += """PsiMod.print_out('\\n')\n"""
        cbsbanners += """banner(' CBS Computation: %s / %s ')\n""" % (mc['f_wfn'].upper(), mc['f_basis'].upper())
        cbsbanners += """PsiMod.print_out('\\n')\n\n"""
        exec cbsbanners

        # Build string of molecule and commands that are dependent on the database
        commands  = '\n'
        commands += """\nPsiMod.set_global_option('BASIS', '%s')\n""" % (mc['f_basis'])
        exec commands

        # Make energy() call
        mc['f_energy'] = call_function_in_1st_argument(func, **kwargs)

        # Fill in energies for subsumed methods
        for menial in VARH[mc['f_wfn']]:
            temp_wfn, temp_portion = split_menial(menial)
            for job in JOBS_EXT:
                if (temp_wfn == job['f_wfn']) and (temp_portion == job['f_portion']) and (mc['f_basis'] == job['f_basis']):
                    job['f_energy'] = PsiMod.get_variable(VARH[temp_wfn][menial])

        PsiMod.clean()

    # Build string of title banner
    cbsbanners = ''
    cbsbanners += """PsiMod.print_out('\\n')\n"""
    cbsbanners += """banner(' CBS Results ')\n"""
    cbsbanners += """PsiMod.print_out('\\n')\n\n"""
    exec cbsbanners

    # Insert obtained energies into the array that stores the cbs stages
    for stage in GRAND_NEED:
        for lvl in stage['d_need'].iteritems():
            MODELCHEM.append(lvl[1])

            for job in JOBS_EXT:
                if ((lvl[1]['f_wfn'] == job['f_wfn']) and (lvl[1]['f_portion'] == job['f_portion']) and
                   (lvl[1]['f_basis'] == job['f_basis'])):
                    lvl[1]['f_energy'] = job['f_energy']

    for stage in GRAND_NEED:
        stage['d_energy'] = call_function_in_1st_argument(stage['d_scheme'], needname = stage['d_need'], mode = 'evaluate')
        finalenergy += stage['d_energy'] * stage['d_coef']

    # Build string of results table
    table_delimit = '  ' + '-' * 105 + '\n'
    tables  = ''
    tables += """\n   ==> %s <==\n\n""" % ('Components')
    tables += table_delimit
    tables += """     %6s %20s %1s %-26s %3s %16s   %-s\n""" % ('', 'Method', '/', 'Basis', 'Rqd', 'Energy [H]', 'Variable')
    tables += table_delimit
    for job in JOBS_EXT:
        star = ''
        for mc in MODELCHEM:
            if (job['f_wfn'] == mc['f_wfn']) and (job['f_basis'] == mc['f_basis']):
                star = '*'
        tables += """     %6s %20s %1s %-27s %2s %16.8f   %-s\n""" % ('', job['f_wfn'], 
                  '/', job['f_basis'], star, job['f_energy'], VARH[job['f_wfn']][job['f_wfn']+job['f_portion']]) 
    tables += table_delimit

    tables += """\n   ==> %s <==\n\n""" % ('Stages')
    tables += table_delimit
    tables += """     %6s %20s %1s %-27s %2s %16s   %-s\n""" % ('Stage', 'Method', '/', 'Basis', 'Wt', 'Energy [H]', 'Scheme')
    tables += table_delimit
    for stage in GRAND_NEED:
        tables += """     %6s %20s %1s %-27s %2d %16.8f   %-s\n""" % (stage['d_stage'], stage['d_wfn'], 
                  '/', stage['d_basis'], stage['d_coef'], stage['d_energy'], stage['d_scheme'].__name__) 
    tables += table_delimit

    tables += """\n   ==> %s <==\n\n""" % ('CBS')
    tables += table_delimit
    tables += """     %6s %20s %1s %-27s %2s %16s   %-s\n""" % ('Stage', 'Method', '/', 'Basis', '', 'Energy [H]', 'Scheme')
    tables += table_delimit
    if do_scf:
        tables += """     %6s %20s %1s %-27s %2s %16.8f   %-s\n""" % (GRAND_NEED[0]['d_stage'], GRAND_NEED[0]['d_wfn'], 
                  '/', GRAND_NEED[0]['d_basis'], '', GRAND_NEED[0]['d_energy'], GRAND_NEED[0]['d_scheme'].__name__) 
    if do_corl:
        tables += """     %6s %20s %1s %-27s %2s %16.8f   %-s\n""" % (GRAND_NEED[1]['d_stage'], GRAND_NEED[1]['d_wfn'], 
                  '/', GRAND_NEED[1]['d_basis'], '', GRAND_NEED[1]['d_energy'], GRAND_NEED[1]['d_scheme'].__name__) 
    if do_delta:
        tables += """     %6s %20s %1s %-27s %2s %16.8f   %-s\n""" % (GRAND_NEED[2]['d_stage'], GRAND_NEED[2]['d_wfn'] + ' - ' + GRAND_NEED[3]['d_wfn'],
                  '/', GRAND_NEED[2]['d_basis'], '', GRAND_NEED[2]['d_energy'] - GRAND_NEED[3]['d_energy'], GRAND_NEED[2]['d_scheme'].__name__) 
    if do_delta2:
        tables += """     %6s %20s %1s %-27s %2s %16.8f   %-s\n""" % (GRAND_NEED[4]['d_stage'], GRAND_NEED[4]['d_wfn'] + ' - ' + GRAND_NEED[5]['d_wfn'],
                  '/', GRAND_NEED[4]['d_basis'], '', GRAND_NEED[4]['d_energy'] - GRAND_NEED[5]['d_energy'], GRAND_NEED[4]['d_scheme'].__name__) 
    tables += """     %6s %20s %1s %-27s %2s %16.8f   %-s\n""" % ('total', 'CBS', '', '', '', finalenergy, '')
    tables += table_delimit
     
    #print tables
    PsiMod.print_out(tables)

    # Restore molecule and options
    #PsiMod.set_local_option('SCF', "WFN", user_wfn)    # TODO refuses to set global option WFN - rejects SCF as option
    PsiMod.set_global_option('BASIS', user_basis)

    PsiMod.set_global_option('WFN', user_wfn)
    if not b_user_wfn:
        PsiMod.revoke_global_option_changed('WFN')

    PsiMod.set_variable('CBS REFERENCE ENERGY', GRAND_NEED[0]['d_energy'])
    PsiMod.set_variable('CBS CORRELATION ENERGY', finalenergy-GRAND_NEED[0]['d_energy'])
    PsiMod.set_variable('CBS TOTAL ENERGY', finalenergy)
    PsiMod.set_variable('CURRENT REFERENCE ENERGY', GRAND_NEED[0]['d_energy'])
    PsiMod.set_variable('CURRENT CORRELATION ENERGY', finalenergy-GRAND_NEED[0]['d_energy'])
    PsiMod.set_variable('CURRENT ENERGY', finalenergy)
    return finalenergy


# Transform and validate basis sets from 'cc-pV[Q5]Z' into [cc-pVQZ, cc-pV5Z] and [4, 5]
def validate_bracketed_basis(basisstring):

    ZETA = ['d', 't', 'q', '5', '6']
    BSET = []
    ZSET = []
    if re.match(r'.*cc-.*\[.*\].*z$', basisstring, flags=re.IGNORECASE):
        basispattern = re.compile(r'^(.*)\[(.*)\](.*)$')
        basisname = basispattern.match(basisstring)
        for b in basisname.group(2):
            if b not in ZETA:
                raise ValidationError('Basis set \'%s\' has invalid zeta level \'%s\'.' % (basisstring, b))
            if len(ZSET) != 0:
                if (int(ZSET[len(ZSET)-1]) - ZETA.index(b)) != 1:
                    raise ValidationError('Basis set \'%s\' has out-of-order zeta level \'%s\'.' % (basisstring, b))
            BSET.append(basisname.group(1) + b + basisname.group(3))
            if b == 'd': b = '2'
            if b == 't': b = '3'
            if b == 'q': b = '4'
            ZSET.append(int(b))
    elif re.match(r'.*\[.*\].*$', basisstring, flags=re.IGNORECASE):
        raise ValidationError('Basis set surrounding series indicator [] in \'%s\' is invalid.'  % (basisstring))
    else:
        BSET.append(basisstring)
        ZSET.append(0)

    return [BSET, ZSET]


# Reform string basis set descriptor from basis set strings, 'cc-pv[q5]z' from [cc-pvqz, cc-pv5z]
def reconstitute_bracketed_basis(needarray):

    ZETA = {'d': 2, 't': 3, 'q': 4, '5': 5, '6': 6}
    ZSET = ['']*len(ZETA)
    BSET = []

    for lvl in needarray.iteritems():
        BSET.append(lvl[1]['f_basis'])

    if (len(BSET) == 1):
        basisstring = BSET[0]
    else:
        indx = 0
        while indx < len(BSET[0]):
            if (BSET[0][indx] != BSET[1][indx]):
                zetaindx = indx
            indx += 1
        for basis in BSET:
            ZSET[ZETA[basis[zetaindx]]-2] = basis[zetaindx]

        pre = BSET[0][:zetaindx]
        post = BSET[0][zetaindx+1:]
        basisstring = pre + '[' + ''.join(ZSET) + ']' + post

    return basisstring


# Defining equation in LaTeX:  $E_{total}(\ell_{max}) =$ 
def highest_1(**largs):

    energypiece = 0.0
    functionname = sys._getframe().f_code.co_name
    f_fields = ['f_wfn', 'f_portion', 'f_basis', 'f_zeta', 'f_energy']
    [mode, NEED, wfnname, BSET, ZSET] = validate_scheme_args(functionname, **largs)

    if (mode == 'requisition'):

        # Impose restrictions on zeta sequence
        if (len(ZSET) == 0):
            raise ValidationError('Call to \'%s\' not valid with \'%s\' basis sets.' % (functionname, len(ZSET)))

        # Return array that logs the requisite jobs
        if (wfnname == 'scf'):
            portion = 'tot'
        else:
            portion = 'corl'
        NEED = {'HI' : dict(zip(f_fields, [wfnname, portion, BSET[len(ZSET)-1], ZSET[len(ZSET)-1], 0.0])) }

        return NEED 

    elif (mode == 'evaluate'):

        # Extract required energies and zeta integers from array
        # Compute extrapolated energy
        energypiece = NEED['HI']['f_energy']

        # Output string with extrapolation parameters
        cbsscheme  = ''
        cbsscheme += """\n   ==> %s <==\n\n""" % (functionname)
        if (NEED['HI']['f_wfn'] == 'scf'):
            cbsscheme += """   HI-zeta (%s) Total Energy:        %16.8f\n""" % (str(NEED['HI']['f_zeta']), energypiece)
        else:
            cbsscheme += """   HI-zeta (%s) Correlation Energy:  %16.8f\n""" % (str(NEED['HI']['f_zeta']), energypiece)
        PsiMod.print_out(cbsscheme)

        return energypiece

   
# Defining equation in LaTeX:  $E_{corl}^{X} = E_{corl}^{\infty} + \beta X^{-3}$
# Solution equation in LaTeX:  $E_{corl}^{\infty} = \frac{E_{corl}^{X} X^3 - E_{corl}^{X-1} (X-1)^3}{X^3 - (X-1)^3}$
# Solution equation in LaTeX:  $\beta = \frac{E_{corl}^{X} - E_{corl}^{X-1}}{X^{-3} - (X-1)^{-3}}$
def corl_xtpl_helgaker_2(**largs):

    energypiece = 0.0
    functionname = sys._getframe().f_code.co_name
    f_fields = ['f_wfn', 'f_portion', 'f_basis', 'f_zeta', 'f_energy']
    [mode, NEED, wfnname, BSET, ZSET] = validate_scheme_args(functionname, **largs)

    if (mode == 'requisition'):

        # Impose restrictions on zeta sequence
        if (len(ZSET) != 2):
            raise ValidationError('Call to \'%s\' not valid with \'%s\' basis sets.' % (functionname, len(ZSET)))

        # Return array that logs the requisite jobs
        NEED = {'HI' : dict(zip(f_fields, [wfnname, 'corl', BSET[1], ZSET[1], 0.0])),
                'LO' : dict(zip(f_fields, [wfnname, 'corl', BSET[0], ZSET[0], 0.0])) }

        return NEED

    elif (mode == 'evaluate'):

        # Extract required energies and zeta integers from array
        eHI = NEED['HI']['f_energy']
        zHI = NEED['HI']['f_zeta']
        eLO = NEED['LO']['f_energy']
        zLO = NEED['LO']['f_zeta']

        # Compute extrapolated energy
        energypiece = (eHI * zHI**3 - eLO * zLO**3) / (zHI**3 - zLO**3) 
        beta = (eHI - eLO) / (zHI**(-3) - zLO**(-3))

        # Output string with extrapolation parameters
        cbsscheme  = ''
        cbsscheme += """\n   ==> %s <==\n\n""" % (functionname)
        cbsscheme += """   LO-zeta (%s) Correlation Energy:  %16.8f\n""" % (str(zLO), eLO)
        cbsscheme += """   HI-zeta (%s) Correlation Energy:  %16.8f\n""" % (str(zHI), eHI)
        cbsscheme += """   Extrapolated Correlation Energy: %16.8f\n""" % (energypiece)
        cbsscheme += """   Beta (coefficient) Value:        %16.8f\n""" % (beta)
        PsiMod.print_out(cbsscheme)

        return energypiece


# Defining equation in LaTeX:  $E_{scf}(\ell_{max}) = E_{scf}^{\text{CBS}} + Ae^{b\ell_{max}}$
def scf_xtpl_helgaker_3(**largs):

    energypiece = 0.0
    functionname = sys._getframe().f_code.co_name
    f_fields = ['f_wfn', 'f_portion', 'f_basis', 'f_zeta', 'f_energy']
    [mode, NEED, wfnname, BSET, ZSET] = validate_scheme_args(functionname, **largs)

    if (mode == 'requisition'):

        # Impose restrictions on zeta sequence
        if (len(ZSET) != 3):
            raise ValidationError('Call to \'%s\' not valid with \'%s\' basis sets.' % (functionname, len(ZSET)))

        # Return array that logs the requisite jobs
        NEED = {'HI' : dict(zip(f_fields, [wfnname, 'tot', BSET[2], ZSET[2], 0.0])),
                'MD' : dict(zip(f_fields, [wfnname, 'tot', BSET[1], ZSET[1], 0.0])),
                'LO' : dict(zip(f_fields, [wfnname, 'tot', BSET[0], ZSET[0], 0.0])) }

        return NEED

    elif (mode == 'evaluate'):

        # Extract required energies and zeta integers from array
        eHI = NEED['HI']['f_energy']
        eMD = NEED['MD']['f_energy']
        eLO = NEED['LO']['f_energy']
        zHI = NEED['HI']['f_zeta']
        zMD = NEED['MD']['f_zeta']
        zLO = NEED['LO']['f_zeta']

        # Compute extrapolated energy
        ratio = (eHI - eMD) / (eMD - eLO)
        alpha = -1 * math.log(ratio)
        beta = (eHI - eMD) / (math.exp(-1 * alpha * zMD) * (ratio - 1))
        energypiece = eHI - beta * math.exp(-1 * alpha * zHI)

        # Output string with extrapolation parameters
        cbsscheme  = ''
        cbsscheme += """\n   ==> %s <==\n\n""" % (functionname)
        cbsscheme += """   LO-zeta (%s) Correlation Energy:  %16.8f\n""" % (str(zLO), eLO)
        cbsscheme += """   MD-zeta (%s) Correlation Energy:  %16.8f\n""" % (str(zMD), eMD)
        cbsscheme += """   HI-zeta (%s) Correlation Energy:  %16.8f\n""" % (str(zHI), eHI)
        cbsscheme += """   Extrapolated Correlation Energy: %16.8f\n""" % (energypiece)
        cbsscheme += """   Alpha (exponent) Value:          %16.8f\n""" % (alpha)
        cbsscheme += """   Beta (coefficient) Value:        %16.8f\n""" % (beta)
        PsiMod.print_out(cbsscheme)

        return energypiece


# Defining equation in LaTeX:  $E_{scf}(\ell_{max}) = E_{scf}^{\text{CBS}} + Ae^{b\ell_{max}}$
def scf_xtpl_helgaker_2(**largs):

    energypiece = 0.0
    functionname = sys._getframe().f_code.co_name
    f_fields = ['f_wfn', 'f_portion', 'f_basis', 'f_zeta', 'f_energy']
    [mode, NEED, wfnname, BSET, ZSET] = validate_scheme_args(functionname, **largs)

    if (mode == 'requisition'):

        # Impose restrictions on zeta sequence
        if (len(ZSET) != 2):
            raise ValidationError('Call to \'%s\' not valid with \'%s\' basis sets.' % (functionname, len(ZSET)))

        # Return array that logs the requisite jobs
        NEED = {'HI' : dict(zip(f_fields, [wfnname, 'tot', BSET[1], ZSET[1], 0.0])),
                'LO' : dict(zip(f_fields, [wfnname, 'tot', BSET[0], ZSET[0], 0.0])) }

        return NEED

    elif (mode == 'evaluate'):

        # Extract required energies and zeta integers from array
        eHI = NEED['HI']['f_energy']
        eLO = NEED['LO']['f_energy']
        zHI = NEED['HI']['f_zeta']
        zLO = NEED['LO']['f_zeta']

        # LAB TODO add ability to pass alternate parameter values in

        # Return extrapolated energy
        alpha = 1.63
        beta = (eHI - eLO) / (math.exp(-1 * alpha * zLO) * (math.exp(-1 * alpha) - 1))
        energypiece = eHI - beta * math.exp(-1 * alpha * zHI)

        # Output string with extrapolation parameters
        cbsscheme  = ''
        cbsscheme += """\n   ==> %s <==\n\n""" % (functionname)
        cbsscheme += """   LO-zeta (%s) Correlation Energy:  %16.8f\n""" % (str(zLO), eLO)
        cbsscheme += """   HI-zeta (%s) Correlation Energy:  %16.8f\n""" % (str(zHI), eHI)
        cbsscheme += """   Extrapolated Correlation Energy: %16.8f\n""" % (energypiece)
        cbsscheme += """   Alpha (exponent) Value:          %16.8f\n""" % (alpha)
        cbsscheme += """   Beta (coefficient) Value:        %16.8f\n""" % (beta)
        PsiMod.print_out(cbsscheme)

        return energypiece


def validate_scheme_args(functionname, **largs):

    mode = ''
    NEED = []
    wfnname = ''
    BSET = []
    ZSET = []

    # Mode where function fills out a form NEED with the computations needed to fulfill its call
    if re.match(r'^requisition$', largs['mode'].lower()):
        mode = largs['mode'].lower()

        if(largs.has_key('wfnname')):
            wfnname = largs['wfnname']
        else:
            raise ValidationError('Call to \'%s\' has keyword \'wfnname\' missing.' % (functionname))

        if re.match(r'scf_.*$', functionname) and (wfnname != 'scf'):
            raise ValidationError('Call to \'%s\' is intended for scf portion of calculation.' % (functionname))
        if re.match(r'corl_.*$', functionname) and (wfnname == 'scf'):
            raise ValidationError('Call to \'%s\' is not intended for scf portion of calculation.' % (functionname))

        if(largs.has_key('basisname')):
            BSET = largs['basisname']
        else:
            raise ValidationError('Call to \'%s\' has keyword \'basisname\' missing.' % (functionname))

        if(largs.has_key('basiszeta')):
            ZSET = largs['basiszeta']
        else:
            raise ValidationError('Call to \'%s\' has keyword \'basiszeta\' missing.' % (functionname))

    # Mode where function reads the now-filled-in energies from that same form and performs the sp, xtpl, delta, etc.
    elif re.match(r'^evaluate$', largs['mode'].lower()):
        mode = largs['mode'].lower()

        if(largs.has_key('needname')):
            NEED = largs['needname']
        else:
            raise ValidationError('Call to \'%s\' has keyword \'needname\' missing.' % (functionname))

    else:
        raise ValidationError('Call to \'%s\' has keyword \'mode\' missing or invalid.' % (functionname))

    return [mode, NEED, wfnname, BSET, ZSET]


def split_menial(menial):

    PTYP = ['tot', 'corl']
    for temp in PTYP:
        if menial.endswith(temp):
            temp_wfn = menial[:-len(temp)]
            temp_portion = temp

    return [temp_wfn, temp_portion]


##  Aliases  ##
cbs = complete_basis_set

#################################
##  End of Complete Basis Set  ##
#################################

