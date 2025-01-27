/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "udf_decl.h"
#include "udf_sb.h"
#include "udf_fs_i.h"
#include "udf_fs.h"
#include "fs.h"
#include "slab.h"
#include "buffer_head.h"

uint32_t udf_get_pblock(struct super_block *sb, uint32_t block,
                        uint16_t partition, uint32_t offset)
{
    struct udf_sb_info *sbi = UDF_SB(sb);
    struct udf_part_map *map;
    if (partition >= sbi->s_partitions)
    {
        udf_debug("block=%d, partition=%d, offset=%d: "
                  "invalid partition\n", block, partition, offset);
        return 0xFFFFFFFF;
    }
    map = &sbi->s_partmaps[partition];
    if (map->s_partition_func)
    {
        return map->s_partition_func(sb, block, partition, offset);
    }
    else
    {
        return map->s_partition_root + block + offset;
    }
}

uint32_t udf_get_pblock_virt15(struct super_block *sb, uint32_t block,
                               uint16_t partition, uint32_t offset)
{
    struct buffer_head *bh = NULL;
    uint32_t newblock;
    uint32_t index;
    uint32_t loc;
    struct udf_sb_info *sbi = UDF_SB(sb);
    struct udf_part_map *map;
    struct udf_virtual_data *vdata;
    struct udf_inode_info *iinfo = UDF_I(sbi->s_vat_inode);

    map = &sbi->s_partmaps[partition];
    vdata = &map->s_type_specific.s_virtual;

    if (block > vdata->s_num_entries)
    {
        udf_debug("Trying to access block beyond end of VAT "
                  "(%d max %d)\n", block, vdata->s_num_entries);
        return 0xFFFFFFFF;
    }

    if (iinfo->i_alloc_type == ICBTAG_FLAG_AD_IN_ICB)
    {
        loc = le32_to_cpu(((__le32 *)(iinfo->i_ext.i_data +
                                      vdata->s_start_offset))[block]);
        goto translate;
    }
    index = (sb->s_blocksize - vdata->s_start_offset) / sizeof(uint32_t);
    if (block >= index)
    {
        block -= index;
        newblock = 1 + (block / (sb->s_blocksize / sizeof(uint32_t)));
        index = block % (sb->s_blocksize / sizeof(uint32_t));
    }
    else
    {
        newblock = 0;
        index = vdata->s_start_offset / sizeof(uint32_t) + block;
    }

    loc = udf_block_map(sbi->s_vat_inode, newblock);

    bh = sb_bread(sb, loc);
    if (!bh)
    {
        udf_debug("get_pblock(UDF_VIRTUAL_MAP:%p,%d,%d) VAT: %d[%d]\n",
                  sb, block, partition, loc, index);
        return 0xFFFFFFFF;
    }

    loc = le32_to_cpu(((__le32 *)bh->b_data)[index]);

    brelse(bh);

translate:
    if (iinfo->i_location.partitionReferenceNum == partition)
    {
        udf_debug("recursive call to udf_get_pblock!\n");
        return 0xFFFFFFFF;
    }

    return udf_get_pblock(sb, loc,
                          iinfo->i_location.partitionReferenceNum,
                          offset);
}

inline uint32_t udf_get_pblock_virt20(struct super_block *sb, uint32_t block,
                                      uint16_t partition, uint32_t offset)
{
    return udf_get_pblock_virt15(sb, block, partition, offset);
}

uint32_t udf_get_pblock_spar15(struct super_block *sb, uint32_t block,
                               uint16_t partition, uint32_t offset)
{
    int i;
    struct sparingTable *st = NULL;
    struct udf_sb_info *sbi = UDF_SB(sb);
    struct udf_part_map *map;
    uint32_t packet;
    struct udf_sparing_data *sdata;

    map = &sbi->s_partmaps[partition];
    sdata = &map->s_type_specific.s_sparing;
    packet = (block + offset) & ~(sdata->s_packet_len - 1);

    for (i = 0; i < 4; i++)
    {
        if (sdata->s_spar_map[i] != NULL)
        {
            st = (struct sparingTable *)
                 sdata->s_spar_map[i]->b_data;
            break;
        }
    }

    if (st)
    {
        for (i = 0; i < le16_to_cpu(st->reallocationTableLen); i++)
        {
            struct sparingEntry *entry = &st->mapEntry[i];
            u32 origLoc = le32_to_cpu(entry->origLocation);
            if (origLoc >= 0xFFFFFFF0)
            {
                break;
            }
            else if (origLoc == packet)
                return le32_to_cpu(entry->mappedLocation) +
                       ((block + offset) &
                        (sdata->s_packet_len - 1));
            else if (origLoc > packet)
            {
                break;
            }
        }
    }

    return map->s_partition_root + block + offset;
}

static uint32_t udf_try_read_meta(struct inode *inode, uint32_t block,
                                  uint16_t partition, uint32_t offset)
{
    struct super_block *sb = inode->i_sb;
    struct udf_part_map *map;
    struct kernel_lb_addr eloc;
    uint32_t elen;
    sector_t ext_offset;
    struct extent_position epos = {};
    uint32_t phyblock;

    if (inode_bmap(inode, block, &epos, &eloc, &elen, &ext_offset) !=
        (EXT_RECORDED_ALLOCATED >> 30))
    {
        phyblock = 0xFFFFFFFF;
    }
    else
    {
        map = &UDF_SB(sb)->s_partmaps[partition];
        /* map to sparable/physical partition desc */
        phyblock = udf_get_pblock(sb, eloc.logicalBlockNum,
                                  map->s_partition_num, ext_offset + offset);
    }

    brelse(epos.bh);
    return phyblock;
}

uint32_t udf_get_pblock_meta25(struct super_block *sb, uint32_t block,
                               uint16_t partition, uint32_t offset)
{
    struct udf_sb_info *sbi = UDF_SB(sb);
    struct udf_part_map *map;
    struct udf_meta_data *mdata;
    uint32_t retblk;
    struct inode *inode;

    udf_debug("READING from METADATA\n");

    map = &sbi->s_partmaps[partition];
    mdata = &map->s_type_specific.s_metadata;
    inode = mdata->s_metadata_fe ? : mdata->s_mirror_fe;

    /* We shouldn't mount such media... */
    BUG_ON(!inode);
    retblk = udf_try_read_meta(inode, block, partition, offset);
    if (retblk == 0xFFFFFFFF)
    {
        udf_warning("error reading from METADATA, "
                    "trying to read from MIRROR");
        inode = mdata->s_mirror_fe;
        if (!inode)
        {
            return 0xFFFFFFFF;
        }
        retblk = udf_try_read_meta(inode, block, partition, offset);
    }

    return retblk;
}

#if defined CONFIG_FSYS_UDFFS_RW
int udf_relocate_blocks(struct super_block *sb, long old_block, long *new_block)
{
    struct udf_sparing_data *sdata;
    struct sparingTable *st = NULL;
    struct sparingEntry mapEntry;
    uint32_t packet;
    int i, j, k, l;
    struct udf_sb_info *sbi = UDF_SB(sb);
    u16 reallocationTableLen;
    struct buffer_head *bh;

    for (i = 0; i < sbi->s_partitions; i++)
    {
        struct udf_part_map *map = &sbi->s_partmaps[i];
        if (old_block > map->s_partition_root &&
            old_block < map->s_partition_root + map->s_partition_len)
        {
            sdata = &map->s_type_specific.s_sparing;
            packet = (old_block - map->s_partition_root) &
                     ~(sdata->s_packet_len - 1);

            for (j = 0; j < 4; j++)
                if (sdata->s_spar_map[j] != NULL)
                {
                    st = (struct sparingTable *)
                         sdata->s_spar_map[j]->b_data;
                    break;
                }

            if (!st)
            {
                return 1;
            }

            reallocationTableLen =
                le16_to_cpu(st->reallocationTableLen);
            for (k = 0; k < reallocationTableLen; k++)
            {
                struct sparingEntry *entry = &st->mapEntry[k];
                u32 origLoc = le32_to_cpu(entry->origLocation);

                if (origLoc == 0xFFFFFFFF)
                {
                    for (; j < 4; j++)
                    {
                        int len;
                        bh = sdata->s_spar_map[j];
                        if (!bh)
                        {
                            continue;
                        }

                        st = (struct sparingTable *)
                             bh->b_data;
                        entry->origLocation =
                            cpu_to_le32(packet);
                        len =
                            sizeof(struct sparingTable) +
                            reallocationTableLen *
                            sizeof(struct sparingEntry);
                        udf_update_tag((char *)st, len);
                        mark_buffer_dirty(bh);
                    }
                    *new_block = le32_to_cpu(
                                     entry->mappedLocation) +
                                 ((old_block -
                                   map->s_partition_root) &
                                  (sdata->s_packet_len - 1));
                    return 0;
                }
                else if (origLoc == packet)
                {
                    *new_block = le32_to_cpu(
                                     entry->mappedLocation) +
                                 ((old_block -
                                   map->s_partition_root) &
                                  (sdata->s_packet_len - 1));
                    return 0;
                }
                else if (origLoc > packet)
                {
                    break;
                }
            }

            for (l = k; l < reallocationTableLen; l++)
            {
                struct sparingEntry *entry = &st->mapEntry[l];
                u32 origLoc = le32_to_cpu(entry->origLocation);

                if (origLoc != 0xFFFFFFFF)
                {
                    continue;
                }

                for (; j < 4; j++)
                {
                    bh = sdata->s_spar_map[j];
                    if (!bh)
                    {
                        continue;
                    }

                    st = (struct sparingTable *)bh->b_data;
                    mapEntry = st->mapEntry[l];
                    mapEntry.origLocation =
                        cpu_to_le32(packet);
                    memmove(&st->mapEntry[k + 1],
                            &st->mapEntry[k],
                            (l - k) *
                            sizeof(struct sparingEntry));
                    st->mapEntry[k] = mapEntry;
                    udf_update_tag((char *)st,
                                   sizeof(struct sparingTable) +
                                   reallocationTableLen *
                                   sizeof(struct sparingEntry));
                    mark_buffer_dirty(bh);
                }
                *new_block =
                    le32_to_cpu(
                        st->mapEntry[k].mappedLocation) +
                    ((old_block - map->s_partition_root) &
                     (sdata->s_packet_len - 1));
                return 0;
            }

            return 1;
        } /* if old_block */
    }

    if (i == sbi->s_partitions)
    {
        /* outside of partitions */
        /* for now, fail =) */
        return 1;
    }

    return 0;
}
#endif /* FSYS_UDF_RW */