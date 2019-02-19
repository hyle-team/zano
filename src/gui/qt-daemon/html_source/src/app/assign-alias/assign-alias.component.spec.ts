import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { AssignAliasComponent } from './assign-alias.component';

describe('AssignAliasComponent', () => {
  let component: AssignAliasComponent;
  let fixture: ComponentFixture<AssignAliasComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ AssignAliasComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(AssignAliasComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
