# Step 1: Implementing DSATUR algorithm in C
This program shows an example implementation of DSATUR algorithm for initializing a colored graph. The  

## Time Complexity:
For a graph of V vertices and E edges, the implementation iterates through every vertex and then iterates over every edge of that vertex. The complexity can be shown in two different ways for two different cases. 

- When E is far smaller than V * V, which means that the average number of edges of every vertex is less than the total number of vertices, the complexity can be shown as:
$G(V, E) = O(V *$ E)

- When E is close to V * V, which means that every vertex is connected to most of the other vertices, the complexity can be shown as:
$G(V) = O(V^2$)

## Space Complexity:
For a graph of V vertices, the implementation uses 2 2D arrays that are of the size V*V bits, one for storing the edges, and the other for storing the colors of vertices. So, the complexity is:

$G(V) = Θ(V^2$)
