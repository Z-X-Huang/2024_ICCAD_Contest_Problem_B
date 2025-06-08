# 2024_ICCAD_Contest_Problem_B
## 2024 ICCAD Contest - Problem B: Power and Timing Optimization Using Multibit Flip-Flop

---
### 1 Introduction
In advanced semiconductor technology nodes, minimizing power and area is a key concern. A common technique is to replace single-bit flip-flops with multibit flip-flops, which saves area and simplifies power, ground, and clock routing.
This process is known as “multibit flip-flop banking.” However, banking can sometimes worsen timing for critical nets, potentially reducing overall optimization. To address this, “multibit flip-flop debanking”—splitting multibit flip-flops back into single-bit ones—is sometimes necessary.  
  
This contest simulates banking and debanking decisions in virtual designs, requiring contestants to optimize timing, power, and area for each testcase.  
  
<img src="png/banking_debanking.png" width="375" height="150" />

---
### 2 Contest Objective
In this contest, the input includes combinational gates and sequential flip-flops. Contestants must preserve the placement and connections of combinational gates, which cannot be moved or altered. Only flip-flops may be moved, banked, or debanked as needed.  
  
The contestants need to develop a banking & debanking algorithm that banks or debanks flip-flops to optimize timing, power, and area while meeting cell density constraints and avoiding cell overlap.  
  
The cost metrics of calculating timing, power, and area for this contest is as follows:  
  
<img src="png/objective_function.png" width="250" height="50" />

