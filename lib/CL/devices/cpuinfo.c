/* cpuinfo.h - parsing of /proc/cpuinfo for OpenCL device info

   Copyright (c) 2012 Pekka Jääskeläinen
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include <pocl_cl.h>
#include <unistd.h>

#include "cpuinfo.h"

const char* cpuinfo = "/proc/cpuinfo";
#define MAX_CPUINFO_SIZE 64*1024
//#define DEBUG_POCL_CPUINFO

/**
 * Detects the maximum clock frequency of the CPU by parsing the cpuinfo.
 *
 * Assumes all cores have the same max clock freq.
 *
 * @return The clock frequency, or -1 if couldn't figure it out.
 */
int
pocl_cpuinfo_detect_max_clock_frequency() {

  if (access (cpuinfo, R_OK) != 0) 
      return -1;
  else 
    {
      FILE *f = fopen (cpuinfo, "r");
      char contents[MAX_CPUINFO_SIZE];
      int num_read = fread (contents, 1, MAX_CPUINFO_SIZE - 1, f);            
      float freq = 0.0f;
      fclose (f);
      contents[num_read] = '\0';

      /* Count the number of times 'processor' keyword is found which
         should give the number of cores overall in a multiprocessor
         system. In Meego Harmattan on ARM it prints Processor instead of
         processor */
      char* p = contents;
      if ((p = strstr (p, "cpu MHz")) != NULL &&
          (p = strstr (p, ": ")) != NULL)
        {
          if (sscanf (p, ": %f", &freq) == 0)
            {
#ifdef DEBUG_POCL_CPUINFO
              printf ("could not parse the cpu MHz field %f\n", freq);
              puts (p);
#endif
              return -1;
            }           
          else 
            {
#ifdef DEBUG_POCL_CPUINFO
              printf ("max_freq %d\n", (int)freq);
#endif
              return (int)freq;
            }
        }
    } 
  return -1;  
}


/**
 * Detects the number of parallel hardware threads supported by
 * the CPU by parsing the cpuinfo.
 *
 * @return The number of hardware threads, or -1 if couldn't figure it out.
 */
int
pocl_cpuinfo_detect_compute_unit_count() {

  if (access (cpuinfo, R_OK) != 0) 
      return -1;
  else 
    {
      FILE *f = fopen (cpuinfo, "r");
      char contents[MAX_CPUINFO_SIZE];
      int num_read = fread (contents, 1, MAX_CPUINFO_SIZE - 1, f);            
      int cores = 0;
      fclose (f);
      contents[num_read] = '\0';

      /* Count the number of times 'processor' keyword is found which
         should give the number of cores overall in a multiprocessor
         system. In Meego Harmattan on ARM it prints Processor instead of
         processor */
      char* p = contents;
      while ((p = strstr (p, "rocessor")) != NULL) 
        {
          cores++;
          /* Skip to the end of the line. Otherwise causes two cores
             to be detected in case of, for example:
             Processor       : ARMv7 Processor rev 2 (v7l) */
          char* eol = strstr (p, "\n");
          if (eol != NULL)
              p = eol;
          ++p;
        }     
#ifdef DEBUG_POCL_CPUINFO
      printf("total cores %d\n", cores);
#endif
      if (cores == 0)
        return -1;

      int cores_per_cpu = 1;
      p = contents;
      if ((p = strstr (p, "cpu cores")) != NULL)
        {
          if (sscanf (p, ": %d\n", &cores_per_cpu) != 1)
            cores_per_cpu = 1;
#ifdef DEBUG_POCL_CPUINFO
          printf ("cores per cpu %d\n", cores_per_cpu);
#endif
        }

      int siblings = 1;
      p = contents;
      if ((p = strstr (p, "siblings")) != NULL)
        {
          if (sscanf (p, ": %d\n", &siblings) != 1)
            siblings = cores_per_cpu;
#ifdef DEBUG_POCL_CPUINFO
          printf ("siblings %d\n", siblings);
#endif
        }
      if (siblings > cores_per_cpu) {
#ifdef DEBUG_POCL_CPUINFO
        printf ("max threads %d\n", cores*(siblings/cores_per_cpu));
#endif
        return cores*(siblings/cores_per_cpu); /* hardware threading is on */
      } else {
#ifdef DEBUG_POCL_CPUINFO
        printf ("max threads %d\n", cores);
#endif
        return cores; /* only multicore, if not unicore*/
      }      
    } 
  return -1;  
}

void
pocl_cpuinfo_detect_device_info(cl_device_id device) 
{
  if ((device->max_compute_units = pocl_cpuinfo_detect_compute_unit_count()) == -1)
    device->max_compute_units = 0;

  if ((device->max_clock_frequency = pocl_cpuinfo_detect_max_clock_frequency()) == -1)
    device->max_clock_frequency = 0;
}