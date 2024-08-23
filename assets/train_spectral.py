import os
import torch
import torch.nn as nn
import torch.optim as optim
import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from torch.utils.data import DataLoader, TensorDataset

# Check if CUDA is available
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(f"Using device: {device}")

current_dir = os.path.dirname(os.path.abspath(__file__))
train_data_file_path = os.path.join(current_dir, 'color_spectral_xyz.csv')
# Load the CSV file
data = pd.read_csv(train_data_file_path, header=None)

# Split the data into features (XYZ) and target (spectral curve)
y = data.iloc[:, :31].values  # First 31 columns are spectral data
X = data.iloc[:, -3:].values  # Last 3 columns are XYZ values

# Normalize XYZ data to range [0, 1]
X = X / 100.0

# Convert to PyTorch tensors and move to GPU
X = torch.tensor(X, dtype=torch.float32).to(device)
y = torch.tensor(y, dtype=torch.float32).to(device)

# Split into training and testing sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Create DataLoader objects
train_dataset = TensorDataset(X_train, y_train)
train_loader = DataLoader(train_dataset, batch_size=32, shuffle=True)

test_dataset = TensorDataset(X_test, y_test)
test_loader = DataLoader(test_dataset, batch_size=32, shuffle=False)

class SpectralNet(nn.Module):
    def __init__(self):
        super(SpectralNet, self).__init__()
        self.fc1 = nn.Linear(3, 64)
        self.fc2 = nn.Linear(64, 128)
        self.fc3 = nn.Linear(128, 256)
        self.fc4 = nn.Linear(256, 31)
        self.relu = nn.ReLU()

    def forward(self, x):
        x = self.relu(self.fc1(x))
        x = self.relu(self.fc2(x))
        x = self.relu(self.fc3(x))
        x = self.fc4(x)
        return x

model = SpectralNet().to(device)

criterion = nn.MSELoss()
optimizer = optim.Adam(model.parameters(), lr=0.001)

num_epochs = 1000

for epoch in range(num_epochs):
    model.train()
    for batch_X, batch_y in train_loader:
        batch_X, batch_y = batch_X.to(device), batch_y.to(device)
        optimizer.zero_grad()
        outputs = model(batch_X)
        loss = criterion(outputs, batch_y)
        loss.backward()
        optimizer.step()
    
    # Evaluate on test set
    model.eval()
    with torch.no_grad():
        test_loss = 0
        for batch_X, batch_y in test_loader:
            batch_X, batch_y = batch_X.to(device), batch_y.to(device)
            outputs = model(batch_X)
            test_loss += criterion(outputs, batch_y).item()
        test_loss /= len(test_loader)
    
    if (epoch + 1) % 10 == 0:
        print(f'Epoch [{epoch+1}/{num_epochs}], Test Loss: {test_loss:.4f}')

model.eval()
with torch.no_grad():
    sample_XYZ = torch.tensor([[0.5, 0.5, 0.5]], dtype=torch.float32).to(device)  # Example normalized XYZ values
    predicted_spectrum = model(sample_XYZ)
    print("Predicted Spectrum:", predicted_spectrum.cpu().numpy())

    # Convert the model to TorchScript
    scripted_model = torch.jit.script(model.cpu())  # Move model to CPU before scripting

    # Save the TorchScript model
    scripted_model.save('spectral_model_scripted.pt')
    print("TorchScript model saved as 'spectral_model_scripted.pt'")

