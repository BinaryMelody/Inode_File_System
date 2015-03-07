#CS356 Operating System Project Report
***
####Name: Zuoming Zhang
####Student ID: 5120309626

##Implementation of project2
Since BDS, BDC_command, BDC_random, FC is very similar from person to person, so I will include in IDS and FS implementation

###IDS
When cache is set to n, each time it reads a command from BDC_random, it will automatically add 1 to another variable N which is initially set to 0. When a command comes, it will save the command in an array and save cylinder number in another array in their arrive order. Then we divide N by n and divide it to (N+(n-1))/n parts, and organize each n part in the following order: Suppose start position of the whole N schedule is 0, which is initial prev = 0.

* **FCFS:** not change
* **SSTF:** reorder them in the array and finally count the SSTF delay when they are organized.

```C
for(i = 0; i<n; ++i){
    next = min(abs(array[i] – prev)); // here i is in a loop from 0 to n-1
    prev = next;
}
```
* **CLOOK:** In each part which contains n cylinder numbers, my program will set prev as initial number, and choose from those number which are not less than prev and organize them in rising order. If all numbers not less than prev are used up, we will append those which are less than prev in rising order to the queue. For example, prev = 10, and the numbers are 12 7 18 25 13 9 1. The order should be like : (10) 12 13 18 25 1 7 9,Pay attention that prev should not be in the queue.

###FS
I will mostly show it through pictures:

The whole diskfile will look like the following:

<img src="/pictures/Cylinder_Structure.jpg" alt="Cylinder Structure" width="650"/>

Please pay attention that, SuperBlock, which contains the general information such as inode numbers, data block numbers and so on is saved to the last block of the file. The file system start positions are not fixed and they vary according to cylinder and sector numbers. Pay attention that although I write Inode_bitmap_start and Data_bitmap_start in separate blocks, they may have some part in the same block in the file system and data_bitmap is closely appended to inode_bitmap. Also, it is actually a byte map instead of bit map because I use ‘0’ and ‘1’ in char instead of in binary bit. (If we can use C++ library, I will use bitset, which is very convenient!)

Inode size is 64 bytes and data block is 256 bytes, so I can store 4 inodes in a single data block, and (Data_bitmap_start – Inode_bitmap_start) = 4 * (data_block_start – inode_start).

Inode is in the following format:

<img src="/pictures/Inode.jpg" alt="I-node" width="300"/>

Each grid is for 8 bytes and numbers are represented in 8 bytes char format.

Single indirect and double indirect are exactly what the book says, and each is 256 bytes, 8 bytes per node. As following shows:

<img src="/pictures/Double_Indirect_Inode.jpg" alt="Double Indirect_Inode" width="650"/>

Inode can be used both for file and category. File Inode’s pointers contains blocks of file content, while category Inode’s pointers contains blocks of categories. Each sub file or category of a category node is 32 bytes, 1 byte is for differentiate file(1) and category(0). 8 bytes are for sub file or category inode address. And the internal 32 -1 -8 -1 = 22 bytes are for name.(The extra * is taken into consideration). So the first block of a directory may look like the following:

<img src="/pictures/Directory_Block" alt="Directory_Block" width="500"/>

The third slot is a directory node pointer which points to 33333333, the fourth is a file node pointer which points to 44444444. Pay attention that I save “.” and “..” as directory content so that cd . and cd .. can be operated as cd to other ordinary sub directories. The “..” address for root node is itself.

***

##IDS analysis

###Data

###X = 50, Y = 100, N = 200, TrackToTrackDelay = 1
<table align="center" valign="center">
<style>
     td {text-align:center}
</style>
   <tr>
      <th>n</th>
      <th colspan="3">X = 50 , Y = 100, N = 200</th>
      <th colspan="3">2X = 100, 2Y = 200, N = 200</th>
   </tr>
   <tr>
      <td></td>
      <th>FCFS</th>
      <th>SSTF</th>
      <th>CLOOK</th>
      <th>FCFS</th>
      <th>SSTF</th>
      <th>CLOOK</th>
   </tr>
   <tr>
      <th>1</th>
      <td>3241</td>
      <td>3241</td>
      <td>3241</td>
      <td>6563</td>
      <td>6563</td>
      <td>6563</td>
   </tr>
   <tr>
      <th>2</th>
      <td>3241</td>
      <td>2532</td>
      <td>3411</td>
      <td>6563</td>
      <td>5698</td>
      <td>6581</td>
   </tr>
   <tr>
      <th>4</th>
      <td>3241</td>
      <td>2020</td>
      <td>2646</td>
      <td>6563</td>
      <td>4039</td>
      <td>5534</td>
   </tr>
   <tr>
      <th>8</th>
      <td>3241</td>
      <td>1136</td>
      <td>1774</td>
      <td>6563</td>
      <td>2328</td>
      <td>3507</td>
   </tr>
   <tr>
      <th>12</th>
      <td>3241</td>
      <td>782</td>
      <td>1313</td>
      <td>6563</td>
      <td>1861</td>
      <td>2674</td>
   </tr>
   <tr>
      <th>20</th>
      <td>3241</td>
      <td>474</td>
      <td>861</td>
      <td>6563</td>
      <td>1050</td>
      <td>1710</td>
   </tr>
   <tr>
      <th>50</th>
      <td>3241</td>
      <td>190</td>
      <td>324</td>
      <td>6563</td>
      <td>486</td>
      <td>670</td>
   </tr>
   <tr>
      <th>200</th>
      <td>3241</td>
      <td>49</td>
      <td>49</td>
      <td>6563</td>
      <td>99</td>
      <td>99</td>
   </tr>
</table>


###Performance diagram:

<img src="/pictures/Performance_1.jpg" alt="Performance 1" width="650"/>

<img src="/pictures/Performance_2.jpg" alt="Performance 2" width="650"/>


###Diagram analysis:
From the diagrams we can find that when n = 1, all the three algorithm run as FCFS. When n is very small but not equals to 1, CLOOK may even cost more time than FCFS, this is because when no number is larger than prev, it will move to the smallest number in the cache waiting for operation instead of the nearer one, thus wasting a lot of time. But when n is larger, it is becomes more efficient than FCFS.

SSTF is a very fast schedule algorithm, although not the optimal one, but it is very close to the optimal one and is much faster than other schedule algorithms. But when n becomes larger, SSTF does not have so much advantage than CLOOK as it has when n is smaller, so we can guess that when n becomes larger, CLOOK is a very good schedule method since SSTF is difficult to implement in real disk operation.

