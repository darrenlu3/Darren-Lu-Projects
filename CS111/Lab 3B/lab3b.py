#!/usr/bin/env python3
# NAME: Darren Lu
# EMAIL: darrenlu3@ucla.edu
# ID: 205394473

import csv
import sys


class Superblock:
    def __init__(self, total_blocks=0, total_inodes=0, block_size=0, inode_size=0, blocks_per_group=0,
                 inodes_per_group=0,
                 first_unreserved=0, starting_inode=0):
        self.total_blocks = total_blocks
        self.total_inodes = total_inodes
        self.block_size = block_size
        self.inode_size = inode_size
        self.blocks_per_group = blocks_per_group
        self.inodes_per_group = inodes_per_group
        self.first_unreserved = first_unreserved
        self.starting_inode = starting_inode

    def total_blocks(self):
        return self.total_blocks

    def first_legal_block(self):
        return int(self.starting_inode) + (128 * (int(self.total_inodes) - 1)) / (int(self.block_size) + 1)

    def first_legal_block2(self):
        return int(self.starting_inode) + (128 * int(self.total_inodes)) / int(self.block_size) + 1

class Block:
    def __init__(self, block_num, directness, inode, offset, inindirect=0):
        self.block_num = block_num
        self.directness = directness
        self.inode = inode
        self.offset = offset
        self.inindirect=inindirect


class Inode:
    def __init__(self, inode_num, mode, links_num, symbolicname="",parentinode="none"):
        self.inode_num = inode_num
        self.mode = mode
        self.links_num = links_num
        self.references = []
        self.pointers = []
        self.links = 0
        self.symbolicname = symbolicname
        self.parentinode=parentinode


class Dirent:
    def __init__(self, parent_inode, inode, name):
        self.parent_inode = parent_inode
        self.inode = inode
        self.name = name


# read csv file
if len(sys.argv) != 2:
    sys.stderr.write("Incorrect number of arguments. Usage: ./lab3b filename")
    sys.exit(1);
try:
    with open(str(sys.argv[1]), newline='') as inputfile:
        csvdata = csv.reader(inputfile, delimiter=',')
        filesystem = list(csvdata)
except IOError:
    sys.stderr.write("Could not read file: " + str(sys.argv[1]))
    sys.exit(1)

# being processing csv file
# check superblock
superblock = Superblock()
inodes = []
blocks = []
dirents = []
incorrect = 0
for row in filesystem:
    if row[0] == "SUPERBLOCK":
        superblock.total_blocks = row[1]
        superblock.total_inodes = row[2]
        superblock.block_size = row[3]
        superblock.inode_size = row[4]
        superblock.blocks_per_group = row[5]
        superblock.inodes_per_group = row[6]
        superblock.first_unreserved = row[7]
    if row[0] == "GROUP":
        #superblock.total_inodes = row[3]
        superblock.starting_inode = row[8]
    if row[0] == "INODE" and row[2] != "s":
        inode = Inode(row[1], row[3], row[6])
        for i in range(15):
            inode.pointers.append(row[12 + i])
            offset = i
            directness = 0
            if i > 11:
                directness = i - 11
                if directness == 1:
                    offset = 12
                if directness == 2:
                    offset = 268
                if directness == 3:
                    offset = 65804
            if int(row[12 + i]) > 0:
                block = Block(row[12 + i], directness, inode, offset)
                blocks.append(block)
        inodes.append(inode)
    if row[0] == "INODE" and row[2] == "s":
        inode = Inode(row[1], row[3], row[6], row[12])
        inodes.append(inode)
# for block in blocks:
#     print(block.block_num)
for row in filesystem:
    if row[0] == "INDIRECT":
        #print(row)
        block = Block(row[5], str(int(row[2]) - 1), row[1], row[3], 1)
        inblocks = 0
        for block2 in blocks:
            if row[5] in block2.block_num:
                inblocks = 1
        if inblocks == 0:
            #print("BLOCKADDED",block.block_num)
            blocks.append(block)

    if row[0] == "DIRENT":
        #print(row)
        dirent = Dirent(row[1], row[3], row[6])
        #print(dirent.parent_inode,dirent.inode)
        dirents.append(dirent)
        if inode.symbolicname != "\'.\'" and inode.symbolicname != "\'..\'":
            for inode in inodes:
                if inode.inode_num == row[3]:
                    inode.parentinode = dirent.parent_inode


# check free inodes on bitmap are not used
freeinodes = []
for row in filesystem:
    if row[0] == "IFREE":
        freeinodes.append(row[1])
# checking inodes
for inode in inodes:
    if inode.mode == 0 and (inode.inode_num not in freeinodes):
        print("UNALLOCATED INODE", inode.inode_num, "NOT ON FREELIST")
        incorrect = 2
    if inode.mode != 0 and (inode.inode_num in freeinodes):
        print("ALLOCATED INODE", inode.inode_num, "ON FREELIST")
        incorrect = 2
# inodes missing from both free list and inodes
for i in range(int(superblock.first_unreserved), int(superblock.total_inodes) + 1):
    missing = 0
    for inode in inodes:
        if i == int(inode.inode_num):
            missing = 1
    for freeinode in freeinodes:
        if i == int(freeinode):
            missing = 1
    if (missing == 0):
        print("UNALLOCATED INODE", i, "NOT ON FREELIST")
        incorrect = 2

# check free blocks on bitmap are not used
freeblocks = []
for row in filesystem:
    if row[0] == "BFREE":
        freeblocks.append(row[1])
#checking blocks
for block in blocks:
    #invalid blocks
    if int(block.block_num) > int(superblock.total_blocks) or int(block.block_num) < 0:
        if block.directness == 0:
            print("INVALID BLOCK", block.block_num, "IN INODE", block.inode.inode_num, "AT OFFSET", block.offset)
        if block.directness == 1:
            print("INVALID INDIRECT BLOCK", block.block_num, "IN INODE", block.inode.inode_num, "AT OFFSET", block.offset)
        if block.directness == 2:
            print("INVALID DOUBLE INDIRECT BLOCK", block.block_num, "IN INODE", block.inode.inode_num, "AT OFFSET", block.offset)
        if block.directness == 3:
            print("INVALID TRIPLE INDIRECT BLOCK", block.block_num, "IN INODE", block.inode.inode_num, "AT OFFSET", block.offset)
        incorrect = 2
    #reserved blocks
    if int(block.block_num) < superblock.first_legal_block():
        if block.directness == 0:
            print("RESERVED BLOCK", block.block_num, "IN INODE", block.inode.inode_num, "AT OFFSET", block.offset)
        if block.directness == 1:
            print("RESERVED INDIRECT BLOCK", block.block_num, "IN INODE", block.inode.inode_num, "AT OFFSET",
                  block.offset)
        if block.directness == 2:
            print("RESERVED DOUBLE INDIRECT BLOCK", block.block_num, "IN INODE", block.inode.inode_num, "AT OFFSET",
                  block.offset)
        if block.directness == 3:
            print("RESERVED TRIPLE INDIRECT BLOCK", block.block_num, "IN INODE", block.inode.inode_num, "AT OFFSET",
                  block.offset)
        incorrect = 2
    #unreferenced blocks
    in_inode = 0
    duplicate = 0
    for inode in inodes:
        if block.block_num in inode.pointers:
            if (in_inode == 1):
                duplicate = 1
            in_inode = 1
    if (in_inode != 1) and (block.block_num not in freeblocks) and block.inindirect != 1:
        print("UNREFERENCED BLOCK", block.block_num)
        incorrect = 2
    #allocated and free blocks
    if (in_inode == 1) and block.block_num in freeblocks:
        print("ALLOCATED BLOCK", block.block_num, "ON FREELIST")
        incorrect = 2
    #duplicate blocks
    if duplicate == 1:
        if block.directness == 0:
            print("DUPLICATE BLOCK", block.block_num, "IN INODE", block.inode.inode_num, "AT OFFSET", block.offset)
        if block.directness == 1:
            print("DUPLICATE INDIRECT BLOCK", block.block_num, "IN INODE", block.inode.inode_num, "AT OFFSET",
                  block.offset)
        if block.directness == 2:
            print("DUPLICATE DOUBLE INDIRECT BLOCK", block.block_num, "IN INODE", block.inode.inode_num, "AT OFFSET",
                  block.offset)
        if block.directness == 3:
            print("DUPLICATE TRIPLE INDIRECT BLOCK", block.block_num, "IN INODE", block.inode.inode_num, "AT OFFSET",
                  block.offset)
        incorrect = 2
#blocks missing from both free list and blocks
# for block in blocks:
#     print(block.block_num)
#print(freeblocks)
for i in range(int(superblock.first_legal_block2()), int(superblock.total_blocks)):
    missing = 0
    for block in blocks:
        #print(i,block.block_num)
        if i == int(block.block_num):
            #print(i,block.block_num)
            missing = 1
    for freeblock in freeblocks:
        if i == int(freeblock):
            missing = 1
    if (missing == 0):
        print("UNREFERENCED BLOCK", i)
        incorrect = 2

#checking dirents
for dirent in dirents:
    #check link count and unallocated inodes
    for inode in inodes:
        if dirent.inode == inode.inode_num:
            inode.links += 1

    #unallocated inodes
    if dirent.inode in freeinodes and dirent.name != "\'..\'" and dirent.name != "\'.\'":
        print("DIRECTORY INODE", dirent.parent_inode, "NAME", dirent.name, "UNALLOCATED INODE", dirent.inode)
        incorrect = 2

    #isnot self and isnot parent
    if dirent.name == "\'.\'":
        if dirent.parent_inode != dirent.inode:
            print("DIRECTORY INODE", dirent.parent_inode, "NAME", dirent.name, "LINK TO INODE", dirent.inode, "SHOULD BE", dirent.parent_inode)
            incorrect = 2
    if dirent.name == "\'..\'":
        parent2 = 0
        for inode in inodes:
            if inode.inode_num == dirent.parent_inode:
                if dirent.inode == "2" or dirent.parent_inode == "2":
                    grandparent_inode = "2"
                    parent2 = 1
                if parent2 == 0:
                    grandparent_inode = inode.parentinode
                if grandparent_inode != dirent.inode:
                    print("DIRECTORY INODE", dirent.parent_inode, "NAME", dirent.name, "LINK TO INODE", dirent.inode,
                          "SHOULD BE", grandparent_inode)
                    incorrect = 2
                break;

    #invalid inodes
    if int(dirent.inode)< 1 or int(dirent.inode) > int(superblock.total_inodes):
        print("DIRECTORY INODE", dirent.parent_inode, "NAME", dirent.name, "INVALID INODE", dirent.inode)
        incorrect = 2
for inode in inodes:
    if int(inode.mode) > 0 and inode.links != int(inode.links_num):
        print("INODE", inode.inode_num, "HAS", inode.links, "LINKS BUT LINKCOUNT IS", inode.links_num)
        incorrect = 2

sys.exit(incorrect)
