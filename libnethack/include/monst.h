/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Alex Smith, 2014-11-21 */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MONST_H
# define MONST_H

# include "global.h"
# include "coord.h"

/* The weapon_check flag is used two ways:
 * 1) When calling mon_wield_item, is 2-6 depending on what is desired.
 * 2) Between calls to mon_wield_item, is 0 or 1 depending on whether or not
 *    the weapon is known by the monster to be cursed (so it shouldn't bother
 *    trying for another weapon).
 * I originally planned to also use 0 if the monster already had its best
 * weapon, to avoid the overhead of a call to mon_wield_item, but it turns out
 * that there are enough situations which might make a monster change its
 * weapon that this is impractical.  --KAA
 */
# define NO_WEAPON_WANTED 0
# define NEED_WEAPON 1
# define NEED_RANGED_WEAPON 2
# define NEED_HTH_WEAPON 3
# define NEED_PICK_AXE 4
# define NEED_AXE 5
# define NEED_PICK_OR_AXE 6

/* The following flags are used for the second argument to display_minventory
 * in invent.c:
 *
 * MINV_NOLET  If set, don't display inventory letters on monster's inventory.
 * MINV_ALL    If set, display all items in monster's inventory, otherwise
 *             just display wielded weapons and worn items.
 */
# define MINV_NOLET 0x01
# define MINV_ALL   0x02

# ifndef ALIGN_H
#  include "align.h"
# endif

/* strategy flags for mstrategy */
# define STRAT_ARRIVE   0x40000000L     /* just arrived on current level */
# define STRAT_WAITFORU 0x20000000L     /* do nothing until player disturbs */
# define STRAT_CLOSE    0x10000000L     /* do nothing until player adjacent */
# define STRAT_WAITMASK 0x30000000L     /* do nothing until something */
# define STRAT_ESCAPE   0x08000000L     /* heal, or avoid STRAT_GOAL[XY] */

/* strategies where you move to STRAT_GOAL[XY] for the following reasons: */
# define STRAT_GROUND   0x04000000L     /* take an item, or just random moves */
# define STRAT_MONSTR   0x02000000L     /* chase down / attack a monster */
# define STRAT_PLAYER   0x01000000L     /* chase down / attack the player */
# define STRAT_TARGMASK 0x07000000L     /* any of the three above cases */

# define STRAT_NONE     0x00000000L     /* tied to a location or random walk */
# define STRAT_STRATMASK 0x0f000000L
# define STRAT_XMASK    0x00ff0000L
# define STRAT_YMASK    0x0000ff00L
# define STRAT_GOAL     0x000000ffL
# define STRAT_GOALX(s) ((xchar)((s & STRAT_XMASK) >> 16))
# define STRAT_GOALY(s) ((xchar)((s & STRAT_YMASK) >> 8))
#define STRAT(w, x, y, typ) (w | ((long)(x)<<16) | ((long)(y)<<8) | (long)typ)

struct monst {
    struct monst *nmon;
    const struct permonst *data;
    struct level *dlevel;       /* pointer to the level this monster is on */
    struct obj *minvent;
    struct obj *mw;     /* weapon */
    unsigned int m_id;
    int mhp, mhpmax;
    int mspec_used;     /* monster's special ability attack timeout */
    unsigned int mtrapseen;     /* bitmap of traps we've been trapped in */
    unsigned int mlstmv;        /* for catching up with lost time */

    int mstrategy;      /* for monsters with mflag3: current strategy */

    /* migration */
    uchar xyloc, xyflags, xlocale, ylocale;
    /* SAVEBREAK (4.3-beta1 -> 4.3-beta2): remove this */
    unsigned int save_compat_dword;

    short mnum; /* permanent monster index number */
    uchar m_lev;        /* adjusted difficulty level of monster */
    xchar mx, my;
    xchar mux, muy;     /* where the monster thinks you are */
    aligntyp malign;    /* alignment of this monster, relative to the player
                           (positive = good to kill) */
    short moveoffset;   /* how this monster's actions map onto turns */
    unsigned short mintrinsics; /* low 8 correspond to mresists */
    schar mtame;        /* level of tameness, implies peaceful */
    uchar m_ap_type;    /* what mappearance is describing: */
# define M_AP_NOTHING   0       /* mappearance is unused -- monster appears as
                                   itself */
# define M_AP_FURNITURE 1       /* stairs, a door, an altar, etc. */
# define M_AP_OBJECT    2       /* an object */
# define M_AP_MONSTER   3       /* a monster */

    uchar mfrozen;
    uchar mblinded;     /* cansee 0, temp.blinded n, blind 0 */

    unsigned int mappearance;   /* for undetected mimics and the wiz */

    unsigned female:1;  /* is female */
    unsigned minvis:1;  /* currently invisible */
    unsigned invis_blkd:1;      /* invisibility blocked */
    unsigned perminvis:1;       /* intrinsic minvis value */
    unsigned cham:3;    /* shape-changer */
/* note: lycanthropes are handled elsewhere */
# define CHAM_ORDINARY          0       /* not a shapechanger */
# define CHAM_CHAMELEON         1       /* animal */
# define CHAM_DOPPELGANGER      2       /* demi-human */
# define CHAM_SANDESTIN         3       /* demon */
# define CHAM_MAX_INDX          CHAM_SANDESTIN
    unsigned mundetected:1;     /* not seen in present hiding place */

    /* implies one of M1_CONCEAL or M1_HIDE, but not mimic (that is, snake,
       spider, trapper, piercer, eel) */
    unsigned mcan:1;    /* has been cancelled */
    unsigned mburied:1; /* has been buried */
    unsigned mspeed:2;  /* current speed */
    unsigned permspeed:2;       /* intrinsic mspeed value */
    unsigned mrevived:1;        /* has been revived from the dead */
    unsigned mavenge:1; /* did something to deserve retaliation */

    unsigned mflee:1;   /* fleeing */
    unsigned mcansee:1; /* cansee 1, temp.blinded 0, blind 0 */
    unsigned mcanmove:1;        /* paralysis, similar to mblinded */
    unsigned msleeping:1;       /* asleep until woken */
    unsigned mstun:1;   /* stunned (off balance) */
    unsigned mconf:1;   /* confused */
    unsigned mpeaceful:1;       /* does not attack unprovoked */
    unsigned mtrapped:1;        /* trapped in a pit, web or bear trap */

    unsigned mleashed:1;        /* monster is on a leash */
    unsigned msuspicious:1;     /* monster is suspicious of the player */
    /* 1 spare bit */
    unsigned isshk:1;   /* is shopkeeper */
    unsigned isminion:1;        /* is a minion */
    unsigned isgd:1;    /* is guard */
    unsigned ispriest:1;        /* is a priest */
    unsigned iswiz:1;   /* is the Wizard of Yendor */

    uchar mfleetim;     /* timeout for mflee */
    uchar wormno;       /* at most 31 worms on any level */
# define MAX_NUM_WORMS  32      /* wormno could hold larger worm ids, but 32 is 
                                   (still) fine */
    xchar weapon_check;
    int misc_worn_check;

    uchar mnamelth;     /* length of name (following mxlth) */
    uchar mxtyp;        /* type of the following data (MX_* flags in
                           permonst.h) */
    short mxlth;        /* length of following data */
    int meating;        /* monster is eating timeout */
    void *mextra[];     /* monster dependent info */
};

/*
 * Note that mextra[] may correspond to any of a number of structures, which
 * are indicated by some of the other fields.
 *      isgd     ->     struct egd
 *      ispriest ->     struct epri
 *      isshk    ->     struct eshk
 *      isminion ->     struct emin
 *                      (struct epri for roaming priests and angels, which is
 *                       compatible with emin for polymorph purposes)
 *      mtame    ->     struct edog
 *                      (struct epri for guardian angels, which do not eat
 *                       or do other doggy things)
 * Since at most one structure can be indicated in this manner, it is not
 * possible to tame any creatures using the other structures (the only
 * exception being the guardian angels which are tame on creation).
 */

# define dealloc_monst(mon) free((mon))

/* these are in mspeed */
# define MSLOW 1/* slow monster */
# define MFAST 2/* speeded monster */

# define NAME(mtmp)         (((const char *)(mtmp)->mextra) + (mtmp)->mxlth)
# define NAME_MUTABLE(mtmp) (((char *)(mtmp)->mextra) + (mtmp)->mxlth)

# define MON_WEP(mon)     ((mon)->mw)
# define MON_NOWEP(mon)   ((mon)->mw = NULL)

# define DEADMONSTER(mon) ((mon)->mhp < 1)

# define onmap(mon) (isok((mon)->mx, (mon)->my))

/* Is a monster using an item? Used to ensure that buzz() calls the correct kill
 * function.
 */
extern boolean m_using;

/* When a long worm is hit, is the hit on the head or thebody? */
extern boolean notonhead;

/* Extra return value for select_rwep() */
extern struct obj *propellor;

#endif /* MONST_H */
