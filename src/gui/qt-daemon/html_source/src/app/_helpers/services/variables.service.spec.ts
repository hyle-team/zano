import { TestBed } from '@angular/core/testing';

import { VariablesService } from './variables.service';

describe('VariablesService', () => {
  beforeEach(() => TestBed.configureTestingModule({}));

  it('should be created', () => {
    const service: VariablesService = TestBed.get(VariablesService);
    expect(service).toBeTruthy();
  });
});
